# quantize_onnx.py —— float32 ONNX -> QDQ int8（ST Edge AI 认的量化格式）
#
# 为什么必须做这一步（2026-09-16 实测证据）：
#   blink.onnx 是纯 float32，ST Edge AI 编译结果：
#       Total epochs 101  ->  纯软件(SW) 100 / 硬件(HW) 1
#       weights 8.43 MiB，activations 1.41 MiB
#       模型兼容性：❌ Not compatible with STM32 memory constraints
#   浮点激活比 int8 大 4 倍，塞不进片上 SRAM；NPU 硬件也几乎没参与运算。
#   官方例程的输入模型都是量化过的：
#       995 用 033_palm_detection_full_quant_pc_uf_od.tflite / *_int8_pc.tflite
#       992 用 efficientnet_v2B1_240_fft_qdq_int8.onnx
#
# 用法:
#   python quantize_onnx.py --onnx models/blink.onnx --flat eye_photo \
#          --task blink --size 64x128
#
# 产出:
#   models/blink_qdq_int8.onnx     <- 拿这个去 stedgeai generate
#   控制台同时打印 量化前/后 在同一验证集上的准确率对比
#
# 依赖: onnxruntime / onnx / numpy / pillow      **不需要 torch**

import argparse
import os
import sys

import numpy as np
from PIL import Image
from onnxruntime.quantization import (CalibrationDataReader, QuantFormat,
                                      QuantType, quantize_static)

from eye_common import (parse_size, resolve_data_dir, resolve_file_path,
                        scan_flat, stratified_split, task_classes)

_HERE = os.path.dirname(os.path.abspath(__file__))


def preprocess(path, H, W):
    """与训练/推理完全一致的预处理：RGB -> Resize(高x宽) -> [0,1] -> [-1,1] -> NCHW。"""
    img = Image.open(path).convert("RGB").resize((W, H), Image.BILINEAR)
    x = np.asarray(img, dtype=np.float32) / 255.0
    x = (x - 0.5) / 0.5
    x = np.transpose(x, (2, 0, 1))[None, ...]
    return np.ascontiguousarray(x, dtype=np.float32)


class EyeCalibReader(CalibrationDataReader):
    """量化校准数据源：喂一批训练集图片给静态量化器，用来定每层的 scale/zero-point。

    只用**训练集**的图，不动验证集 —— 否则 scale 会朝验证样本偏，评估数字虚高。
    """

    def __init__(self, samples, H, W, input_name, limit=200):
        self.items = samples[:limit]
        self.H, self.W = H, W
        self.input_name = input_name
        self.idx = 0

    def get_next(self):
        if self.idx >= len(self.items):
            return None
        path, _ = self.items[self.idx]
        self.idx += 1
        return {self.input_name: preprocess(path, self.H, self.W)}

    def rewind(self):
        self.idx = 0


def onnx_accuracy(onnx_path, samples, H, W, n_cls):
    """用 onnxruntime 在给定样本上算 准确率 / 每类召回 / 混淆矩阵。"""
    import onnxruntime as ort
    sess = ort.InferenceSession(onnx_path)
    iname = sess.get_inputs()[0].name
    cm = np.zeros((n_cls, n_cls), dtype=int)
    for path, y in samples:
        out = sess.run(None, {iname: preprocess(path, H, W)})[0]
        cm[y, int(out[0].argmax())] += 1
    total = cm.sum()
    acc = float(np.trace(cm)) / max(1, total)
    rec = [cm[i, i] / max(1, cm[i].sum()) for i in range(n_cls)]
    return acc, rec, cm


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--onnx", required=True, help="输入 float32 ONNX")
    ap.add_argument("--flat", required=True,
                    help="采集目录（如 eye_photo），用于抽校准图与验证图")
    ap.add_argument("--out", default=None,
                    help="输出路径，默认 <输入名>_qdq_int8.onnx")
    ap.add_argument("--task", choices=["blink", "gaze"], default="blink")
    ap.add_argument("--size", default="64x128")
    ap.add_argument("--calib-count", type=int, default=200,
                    help="校准图数量（从训练集抽，默认 200）")
    ap.add_argument("--format", choices=["qdq", "qoperator"],
                    default="qdq",
                    help="量化格式。QDQ 是 ST Edge AI 认的格式；"
                         "QOperator 精度更高，但需先确认工具链接受")
    ap.add_argument("--val-split", type=float, default=0.2)
    ap.add_argument("--seed", type=int, default=42,
                    help="必须与训练时一致，否则验证集和训练集重叠")
    args = ap.parse_args()

    args.onnx = resolve_file_path(args.onnx)
    args.flat = resolve_data_dir(args.flat)
    if not os.path.exists(args.onnx):
        raise SystemExit("找不到输入 ONNX: %s" % args.onnx)

    H, W = parse_size(args.size)
    classes = task_classes(args.task)
    n_cls = len(classes)

    if args.out is None:
        args.out = os.path.join(os.path.dirname(args.onnx),
                                os.path.basename(args.onnx).replace(
                                    ".onnx", "_qdq_int8.onnx"))
    args.out = resolve_file_path(args.out)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)

    # ---- 数据：与训练完全相同的划分（同 seed），校准只用训练那部分 ----------
    samples, _, counted = scan_flat(args.flat, args.task)
    if not samples:
        raise SystemExit("没扫到任何图，检查 --flat 指向的目录")
    tr_s, va_s = stratified_split(samples, n_cls, args.val_split, args.seed)
    calib = tr_s[:args.calib_count]
    print("\n总数 %d -> train %d / val %d；校准用 train 里的 %d 张"
          % (len(samples), len(tr_s), len(va_s), len(calib)))
    print("类别顺序:", classes)

    # ---- 量化前基线 ---------------------------------------------------------
    print("\n[1/3] 量化前 float32 精度 ...")
    acc_f, rec_f, cm_f = onnx_accuracy(args.onnx, va_s, H, W, n_cls)
    print("      acc=%.4f  每类召回=%s"
          % (acc_f, [round(r, 3) for r in rec_f]))

    # ---- 静态量化（QDQ int8）------------------------------------------------
    qfmt = (QuantFormat.QOperator if args.format == "qoperator"
            else QuantFormat.QDQ)
    print("\n[2/3] 静态量化 %s int8 ..." % args.format)
    import onnxruntime as ort
    input_name = ort.InferenceSession(args.onnx).get_inputs()[0].name
    reader = EyeCalibReader(calib, H, W, input_name, limit=args.calib_count)
    kw = dict(quant_format=qfmt, per_channel=True,
              weight_type=QuantType.QInt8)
    try:
        quantize_static(args.onnx, args.out, reader,
                        activation_type=QuantType.QInt8, **kw)
    except TypeError:
        # onnxruntime >= 1.17 把 activation_type 改名为 activations_type
        quantize_static(args.onnx, args.out, reader,
                        activations_type=QuantType.QInt8, **kw)
    sz_in = os.path.getsize(args.onnx) / 1048576.0
    sz_out = os.path.getsize(args.out) / 1048576.0
    print("      写出 %s" % args.out)
    if sz_out < sz_in:
        print("      体积 %.2f MiB -> %.2f MiB  (缩小 %.1f 倍)"
              % (sz_in, sz_out, sz_in / max(sz_out, 1e-6)))
    else:
        print("      体积 %.4f MiB -> %.4f MiB"
              % (sz_in, sz_out))
        print("      （模型本身很小时量化后可能反而略大，正常现象；"
              "真正收益在激省内存与 NPU 加速上）")

    # ---- 量化后精度 ---------------------------------------------------------
    print("\n[3/3] 量化后 int8 精度 ...")
    acc_q, rec_q, cm_q = onnx_accuracy(args.out, va_s, H, W, n_cls)
    print("      acc=%.4f  每类召回=%s"
          % (acc_q, [round(r, 3) for r in rec_q]))

    def show(cm, tag):
        print("      %s 混淆矩阵（行=真实 列=预测）" % tag)
        header = "        " + "".join("%9s" % c for c in classes)
        print(header)
        for i, c in enumerate(classes):
            print("        %-8s" % c + "".join("%9d" % v for v in cm[i]))

    print()
    show(cm_f, "float32")
    show(cm_q, "int8   ")

    drop = acc_f - acc_q
    print("\n================= 结论 =================")
    print("  量化前 acc = %.4f" % acc_f)
    print("  量化后 acc = %.4f   掉点 %+.4f" % (acc_q, -drop))
    if drop <= 0.02:
        print("  ✅ 掉点在 2% 以内，可以用")
    elif drop <= 0.05:
        print("  ⚠️ 掉点 2%~5%，勉强可用；不行就加 --calib-count 或换 QAT")
    else:
        print("  ❌ 掉点超过 5%%，别用这个量化模型")
    print("\n下一步（在 eye_ai_train 目录下执行）:")
    print("  python verify_onnx.py %s" % os.path.relpath(args.out, _HERE))
    print("  stedgeai generate --target stm32n6 "
          "--st-neural-art default@user_neuralart_eye.json ^")
    print("     --model %s --no-inputs-allocation --no-outputs-allocation"
          % os.path.relpath(args.out, _HERE))
    print("=======================================")
    return 0 if drop <= 0.05 else 1


if __name__ == "__main__":
    sys.exit(main())
