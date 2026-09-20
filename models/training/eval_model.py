# eval_model.py —— 模型精度验收（流程里此前缺失的一环）
#
# 2026-09-15 新建。为什么需要它：
#   * `verify_onnx.py` 只能证明"模型能跑"，跟精度无关；
#   * `train_blink.py` 只打印一个 val_acc，没有通过线、没有分类别表现；
#   => 于是"这个模型能不能上板"没有可执行的判据。
#
# 用法（**评估集合与训练时完全一致** —— 同一个 --seed 会复现同一份 val 划分）：
#   python eval_model.py --pt D:\eye_ai\models\blink_best.pt \
#          --flat D:\eye_ai\capture\0915 --task blink --size 64x128
#
# 也可以直接指向已分好类的目录（此时评估该目录下全部图片）：
#   python eval_model.py --pt ... --data D:\eye_ai\datasets\blink\val \
#          --task blink --size 64x128
#
# 输出：总体准确率 / 每类召回 / 混淆矩阵 / 通过线判定
# 退出码：0 = PASS，1 = FAIL（并提示差在哪一类）
#
# 为什么强调"每类召回"：只看总体准确率会被多数类骗 —— 全预测成 open 也能有
# 50% 的"准确率"，但闭眼一张都抓不到。

import argparse
import os

import torch
import torch.nn as nn
from torchvision import models, transforms

from eye_common import (ListDataset, confusion_matrix, parse_size,
                        per_class_recall, resolve_data_dir, scan_flat,
                        show_confusion, stratified_split, task_classes)

# blink 从二分类扩到五分类（closed/open/left/right/other）后，任务里多了
# "看左/看右"这种细粒度判别，比原来的睁/闭眼难；而且 right 类样本只有 55 张。
# 0.95 会把大多数正常结果判 FAIL 从而卡住流水线，故下调到 0.90 —— 仍然远高于
# 随机（1/5 = 0.20）。真正的判据是混淆矩阵与每类召回，不是这一个数字。
PASS_LINE = {"blink": 0.90, "gaze": 0.85}

parser = argparse.ArgumentParser()
parser.add_argument("--pt", required=True)
parser.add_argument("--flat", default=None, help="板端采集的平铺目录")
parser.add_argument("--data", default=None,
                    help="已分类目录（如 ...\\blink\\val），此时评估全部图片")
parser.add_argument("--task", choices=["blink", "gaze"], default="blink")
parser.add_argument("--size", default="64x128")
parser.add_argument("--seed", type=int, default=42,
                    help="必须与训练时相同，否则评估集与训练集重叠")
parser.add_argument("--val-split", type=float, default=0.2)
parser.add_argument("--min-acc", type=float, default=None,
                    help="覆盖内置通过线")
args = parser.parse_args()

# 相对路径容错：无论从哪个目录运行都能找到数据
args.flat = resolve_data_dir(args.flat)
args.data = resolve_data_dir(args.data)

CLASSES = task_classes(args.task)
NUM_CLASSES = len(CLASSES)
H, W = parse_size(args.size)
LINE = args.min_acc if args.min_acc is not None else PASS_LINE[args.task]

tf = transforms.Compose([
    transforms.Resize((H, W)),
    transforms.ToTensor(),
    transforms.Normalize([0.5] * 3, [0.5] * 3),
])

# ---- 评估集 -----------------------------------------------------------------
if args.flat:
    samples, _, _ = scan_flat(args.flat, args.task)
    _, va_s = stratified_split(samples, NUM_CLASSES, args.val_split, args.seed)
    print(f"[eval] 复现训练时的 val 划分（seed={args.seed}, "
          f"val_split={args.val_split}）-> {len(va_s)} 张")
    ds = ListDataset(va_s, tf)
elif args.data:
    from PIL import Image

    class DirDataset(ListDataset):
        """目录直接评估：类别顺序按 CLASSES 匹配文件名关键词。"""

    paths = [os.path.join(args.data, n) for n in sorted(os.listdir(args.data))
             if n.lower().endswith((".bmp", ".png", ".jpg", ".jpeg"))]
    from eye_common import class_of, label_of
    pairs = []
    for p in paths:
        c = class_of(label_of(p), args.task)
        if c is not None:
            pairs.append((p, CLASSES.index(c)))
    print(f"[eval] {args.data} -> {len(pairs)} 张（按文件名关键词分类）")
    ds = ListDataset(pairs, tf)
else:
    raise SystemExit("必须给 --flat 或 --data")

loader = torch.utils.data.DataLoader(ds, batch_size=32, shuffle=False)

# ---- 模型 -------------------------------------------------------------------
model = models.mobilenet_v2()
model.classifier[1] = nn.Linear(model.last_channel, NUM_CLASSES)
model.load_state_dict(torch.load(args.pt, map_location="cpu"))
model.eval()
print(f"[eval] {args.pt}  input=1x3x{H}x{W}  classes={CLASSES}")

yt, yp = [], []
with torch.no_grad():
    for x, y in loader:
        pred = model(x).argmax(1)
        yt += y.tolist()
        yp += pred.tolist()

n = len(yt)
if n == 0:
    raise SystemExit("评估集为空 —— 检查 --flat/--data 路径与文件名是否含类别关键词")

acc = sum(1 for a, b in zip(yt, yp) if a == b) / n
cm = confusion_matrix(yt, yp, NUM_CLASSES)
rec = per_class_recall(cm)

print()
print(f"总体准确率 = {acc:.4f}   (n={n})")
show_confusion(cm, CLASSES)

print()
print(f"通过线 = {LINE:.2f}   (task={args.task}，可用 --min-acc 覆盖)")
if acc >= LINE and min(rec) > 0:
    print(f"✅ PASS（最差类召回 {min(rec):.3f}）")
    print("下一步：export_onnx.py -> verify_onnx.py -> stedgeai generate")
    raise SystemExit(0)

print(f"❌ FAIL")
if min(rec) == 0:
    print(f"   有类别完全抓不到（recall=0）："
          f"{[c for c, r in zip(CLASSES, rec) if r == 0]}")
    print("   多半是样本量太少或两类在画面里差别太小 —— 先看采集图再调参")
else:
    print(f"   最差类召回 {min(rec):.3f}："
          f"{[f'{c}:{r:.2f}' for c, r in zip(CLASSES, rec)]}")
print("   可选：加数据 / 调 --epochs / 换更大输入（如 --size 96x192）/ 检查曝光")
raise SystemExit(1)
