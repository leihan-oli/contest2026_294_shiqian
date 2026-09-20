# eye_common.py —— 眼动数据集公共工具（训练 / 评估 / 采集检查 共用）
#
# 2026-09-15 新建。为什么要有这个文件：训练、评估、采集检查三个脚本必须用
# **完全相同的**尺寸解析、类别映射、颜色转换规则，否则会出现
# "训练时是 A、评估/推理时是 B"的隐蔽不一致 —— 这正是会掉精度的"一致性地雷"
# （见 eye_功能补齐方案.md §1.3）。
#
# 颜色约定（全流程唯一真源）：
#   板端 cam_save ... color  -> BMP（存储为 BGR）
#   PIL Image.open(...).convert("RGB")  -> RGB，训练/评估都走这一条
#   板端推理时 DCMIPP 出来是 RGB565 -> 展开成 RGB888（同样是 RGB）
#
# 尺寸约定：输入写 "高x宽"，例如 64x128 = 高 64 / 宽 128（匹配 ROI 300x145 的 ~2:1）

import os
import re
import random

import numpy as np
from PIL import Image

EXTS = (".bmp", ".png", ".jpg", ".jpeg")

# label 关键词 -> 类别名。按小写子串匹配，**顺序敏感**（长词/特例放前面）。
# 板端采集命名建议直接叫 eye_open / eye_closed / gaze_left ...，但这里对
# blink_open / open / close 之类的别名也兼容。
#
# ⚠️ 不要用 "mid" 当 gaze_center 的关键词：标定图叫 cal_mid，会被误收进训练集。
TASK_KEYWORDS = {
    "blink": [
        # 五分类（2026-09-18 起）。类必须互斥，定义见文档：
        #   closed = 闭眼（不论方向）
        #   open   = 睁眼 + 注视正前方
        #   left / right = 睁眼 + 眼球向左/右，头保持不动
        #   other  = 其余一切（眼出框/背景/歪头/眯眼/上下看）
        # 顺序即标签索引，追加/插入都会影响板端解码，改动必须同步
        # apps/examples/eye_cam/eye_cam_main.c 的 EYE_CAM_CLS_* 常量。
        ("closed", ("closed", "close", "shut")),
        ("open",   ("open", "opened")),
        ("left",   ("left",)),
        ("right",  ("right",)),
        # 第三类：框内不是"双眼端正的清晰眼部"。
        # 2026-09-17 新增。为什么必须有：模型原本只有二分类，任何"框里没眼睛"
        # 的输入（侧脸、背景、单眼/歪头、失焦）都只能外推到最近的一类，实测
        # 一律落进 closed —— 眼控系统会把"人没看屏幕"当成"看屏幕且闭眼"。
        # 追加在最后是刻意的：closed=0 / open=1 保持不变，板端只需多读一个索引。
        ("other",  ("other", "none", "idle")),
    ],
    "gaze": [
        ("center", ("center", "centre")),
        ("left",   ("left",)),
        ("right",  ("right",)),
        ("up",     ("up",)),
        ("down",   ("down",)),
    ],
}

# 这些前缀的**不是训练数据**，一律跳过：
#   cal_*    眼位标定图（只用来定 ROI，见主线流程文档）
#   test_/tmp_/debug_/sample_  临时试拍
EXCLUDE_PREFIXES = ("cal", "test", "tmp", "debug", "sample")


def task_classes(task):
    """返回任务的类别名列表（顺序即标签顺序，全流程一致）。"""
    return [c for c, _ in TASK_KEYWORDS[task]]


def resolve_data_dir(p):
    """把用户给的相对路径解析成真正存在的目录。

    优先按当前工作目录（cwd）解析（标准行为）；找不到时再按`本文件所在目录`
    解析。这样既支持 `cd eye_ai_train && python train_blink.py --flat eye_photo`，
    也支持从任何位置 `python eye_ai_train/train_blink.py --flat eye_photo`。
    """
    if not p or os.path.isabs(p) or os.path.isdir(p):
        return p
    alt = os.path.join(os.path.dirname(os.path.abspath(__file__)), p)
    return alt if os.path.isdir(alt) else p


def resolve_file_path(p):
    """文件版路径兜底（用于 --onnx / --pt / --out 这类参数）。"""
    if not p or os.path.isabs(p) or os.path.exists(p):
        return p
    alt = os.path.join(os.path.dirname(os.path.abspath(__file__)), p)
    return alt if os.path.exists(alt) else p


def parse_size(spec):
    """'64x128' -> (64, 128) 即 (高, 宽)；'96' -> (96, 96)。"""
    if isinstance(spec, (tuple, list)):
        return int(spec[0]), int(spec[1])
    s = str(spec).lower().replace(" ", "")
    if "x" in s:
        h, w = s.split("x", 1)
        return int(h), int(w)
    n = int(s)
    return n, n


def label_of(fname):
    """'eye_open_007.bmp' -> 'eye_open'（去掉末尾的 _NNN / -NNN 序号）。"""
    base = os.path.splitext(os.path.basename(fname))[0]
    m = re.match(r"^(.*?)[_-](\d+)$", base)
    return m.group(1) if m else base


def class_of(label, task):
    """把 label 映射到任务类别；无法归类或属于标定/试拍图时返回 None。"""
    low = label.lower()
    if low.startswith(EXCLUDE_PREFIXES):
        return None
    for cls, keys in TASK_KEYWORDS[task]:
        for k in keys:
            if k in low:
                return cls
    return None


def scan_flat(folder, task, verbose=True):
    """扫描采集目录，返回样本清单。

    支持两种布局（2026-09-16 起）：
        (a) 平铺：folder/blink_open_005.bmp、folder/blink_closed_005.bmp
        (b) 一层子目录：folder/open/*.bmp、folder/close/*.bmp
                        （SD 卡按类别分目录拷回来的形态，也是当前 eye_photo/ 的形态）

    自动跳过名字以 `_` 开头的文件与目录 —— 即 check_eye_capture.py 的
    `_rejected/` 隔离区，避免把人工剔除的废片又收回来。

    Returns:
        samples : [(path, label_index), ...]
        classes : 类别名列表
        counted : {类名: 张数}
    """
    classes = task_classes(task)
    idx = {c: i for i, c in enumerate(classes)}
    samples, skipped, counted = [], {}, {}

    def feed(name, parent):
        if name.startswith("_") or not name.lower().endswith(EXTS):
            return
        cls = class_of(label_of(name), task)
        if cls is None:
            lab = label_of(name)
            skipped[lab] = skipped.get(lab, 0) + 1
            return
        samples.append((os.path.join(parent, name), idx[cls]))
        counted[cls] = counted.get(cls, 0) + 1

    for name in sorted(os.listdir(folder)):
        full = os.path.join(folder, name)
        if os.path.isdir(full):
            if name.startswith(("_", ".")):
                continue
            for sub in sorted(os.listdir(full)):
                feed(sub, full)
        else:
            feed(name, folder)

    if verbose:
        print(f"[scan] {folder}")
        for c in classes:
            print(f"   {c:8s}: {counted.get(c, 0)} 张")
        if skipped:
            print(f"   [跳过] 与本任务无关的 label: {skipped}")

    return samples, classes, counted


def stratified_split(samples, n_classes, val_split, seed=42):
    """按类别分层划分 train/val（每类各自切，保证小类不被整类切走）。"""
    rng = random.Random(seed)
    by_cls = {i: [] for i in range(n_classes)}
    for s in samples:
        by_cls[s[1]].append(s)

    tr, va = [], []
    for items in by_cls.values():
        items = items[:]
        rng.shuffle(items)
        n_val = max(1, int(round(len(items) * val_split))) if items else 0
        va += items[:n_val]
        tr += items[n_val:]

    rng.shuffle(tr)
    rng.shuffle(va)
    return tr, va


def check_min_per_class(counted, n_classes, minimum=50):
    """采集量守卫：某类太少时训练必然过拟合，直接提示。"""
    warn = [c for c, n in counted.items() if n < minimum]
    return warn


class ListDataset:
    """极简 Dataset：[(path, label), ...] + torchvision transform。

    独立于 ImageFolder：板端采集是"平铺 + <label>_NNN.bmp"命名，
    不需要先手工搭 train/val 目录树。
    """

    def __init__(self, samples, tf):
        self.samples = samples
        self.tf = tf

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, index):
        path, y = self.samples[index]
        img = Image.open(path).convert("RGB")
        return self.tf(img), y


def confusion_matrix(y_true, y_pred, n_classes):
    """行 = 真实类，列 = 预测类。"""
    cm = np.zeros((n_classes, n_classes), dtype=int)
    for t, p in zip(y_true, y_pred):
        cm[t, p] += 1
    return cm


def per_class_recall(cm):
    """每类召回率；该行无样本时返回 0。"""
    out = []
    for i in range(cm.shape[0]):
        total = cm[i].sum()
        out.append(float(cm[i, i]) / total if total else 0.0)
    return out


def show_confusion(cm, classes):
    """打印混淆矩阵 + 每类召回（只报总体准确率会被多数类骗）。"""
    print("       " + "".join(f"{c[:9]:>10s}" for c in classes) + "   (列 = 预测)")
    for i, c in enumerate(classes):
        row = "".join(f"{v:>10d}" for v in cm[i])
        total = cm[i].sum()
        rec = cm[i, i] / total if total else 0.0
        print(f"{c[:6]:>6s} {row}   recall {rec:.3f}  (n={total})")
