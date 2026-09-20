# train_blink.py —— 眨眼 / 注视方向分类训练 (Windows, PyTorch)
#
# 2026-09-15 改版（彩色 + 非正方形 + 支持板端平铺目录）
#
# 用法 A（推荐）—— 直接把从 SD 卡拷下来的目录丢进来，
#                  脚本按文件名前缀自动分类、按类分层 8:2 划分：
#   python train_blink.py --flat eye_photo --task blink --size 64x128
#   python train_blink.py --flat eye_photo --task gaze  --size 64x128
#   （eye_photo/ 下无论是平铺还是 open//close/ 子目录都能识别；
#     check_eye_capture.py 剔掉的 _rejected/ 会自动跳过）
#
# 用法 B（兼容）—— 已手工分好 train/val 的 ImageFolder 目录：
#   python train_blink.py --data D:\eye_ai\datasets\blink --task blink --size 64x128
#
# 相对旧版改了什么、为什么：
#   1) **彩色**（去掉 Grayscale）：肤色(黄)/眼白(白)/虹膜(棕黑) 在色度上可分；
#      灰度化后三者接近同一灰阶 —— 这正是"皮肤和眼睛差别不大"的根因。
#      已验证的 palm995 走的也是彩色路径（192x192x3）。
#   2) **64x128（高x宽）**：匹配眼部 ROI 300x145 的 ~2:1 视野。旧版整脸缩到
#      96x96 时眼睛只有 ~10x5 像素；现在 ~55x25，眼睑/虹膜细节才可分辨。
#   3) **--flat**：板端 cam_save 把所有帧平铺在 /mnt/sdcard（<label>_NNN.bmp），
#      不必再手工搭 train/val 目录树。
#
# 归一化（必须与板端一致，见 eye_功能补齐方案.md §1.5）：
#   ToTensor() -> [0,1] -> Normalize(0.5,0.5) -> [-1,1]
#   量化阶段由 ST Edge AI 折叠进模型；板端只需保证 BGR->RGB、尺寸、缩放算法一致。

import argparse
import os

import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import DataLoader
from torchvision import datasets, transforms, models

from eye_common import (ListDataset, check_min_per_class, confusion_matrix,
                        parse_size, resolve_data_dir, scan_flat,
                        show_confusion, stratified_split, task_classes)

_HERE = os.path.dirname(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--data", default=None,
                    help="ImageFolder 根目录（内含 train/ 与 val/）")
parser.add_argument("--flat", default=None,
                    help="采集目录（平铺或 open//close/ 子目录均可），"
                         "文件名为 <label>_NNN.bmp")
parser.add_argument("--task", choices=["blink", "gaze"], default="blink")
parser.add_argument("--size", default="64x128",
                    help="模型输入 '高x宽'（如 64x128），或单个整数表示正方形")
parser.add_argument("--epochs", type=int, default=30)
parser.add_argument("--batch", type=int, default=32)
parser.add_argument("--lr", type=float, default=1e-3)
parser.add_argument("--out", default=os.path.join(_HERE, "models"),
                    help="模型输出目录（默认：脚本同级的 models/，与拷贝位置无关）")
parser.add_argument("--val-split", type=float, default=0.2)
parser.add_argument("--seed", type=int, default=42)
args = parser.parse_args()

# 相对路径容错：无论从哪个目录运行都能找到 eye_photo/
args.flat = resolve_data_dir(args.flat)
args.data = resolve_data_dir(args.data)

SIZE_HW = parse_size(args.size)          # (高, 宽)
CLASSES = task_classes(args.task)
NUM_CLASSES = len(CLASSES)
MEAN, STD = 0.5, 0.5

# ---- 数据增强 --------------------------------------------------------------
# 彩色下 ColorJitter 才真正有意义（色度扰动），亮度扰动覆盖 bright/dark 两档预设。
train_tf = transforms.Compose([
    transforms.Resize(SIZE_HW),
    transforms.RandomAffine(degrees=5, translate=(0.05, 0.05)),
    transforms.ColorJitter(brightness=0.25, contrast=0.2, saturation=0.2),
    transforms.ToTensor(),
    transforms.Normalize([MEAN] * 3, [STD] * 3),
])
val_tf = transforms.Compose([
    transforms.Resize(SIZE_HW),
    transforms.ToTensor(),
    transforms.Normalize([MEAN] * 3, [STD] * 3),
])

# ---- 数据集 ----------------------------------------------------------------
if args.flat:
    samples, _, counted = scan_flat(args.flat, args.task)
    warn = check_min_per_class(counted, NUM_CLASSES, minimum=50)
    if warn:
        print(f"⚠️ 以下类别样本偏少（<50），精度会不稳：{warn}")
    tr_s, va_s = stratified_split(samples, NUM_CLASSES, args.val_split, args.seed)
    print(f"总数 {len(samples)} -> train {len(tr_s)} / val {len(va_s)}")
    train_ds = ListDataset(tr_s, train_tf)
    val_ds = ListDataset(va_s, val_tf)
elif args.data:
    train_ds = datasets.ImageFolder(os.path.join(args.data, "train"), train_tf)
    val_ds = datasets.ImageFolder(os.path.join(args.data, "val"), val_tf)
    # 类别顺序必须与全流程一致，否则标签含义会错位。
    if list(train_ds.classes) != CLASSES:
        print(f"⚠️ ImageFolder 类别顺序 {train_ds.classes} != 期望 {CLASSES}"
              f" —— 请把目录名改成期望的类名，否则标签会错位")
else:
    raise SystemExit("必须给 --flat（板端采集目录）或 --data（ImageFolder 根目录）")

print("类别顺序:", CLASSES)
train_loader = DataLoader(train_ds, batch_size=args.batch, shuffle=True,
                          num_workers=0)
val_loader = DataLoader(val_ds, batch_size=args.batch, shuffle=False,
                        num_workers=0)

# ---- 模型 ------------------------------------------------------------------
model = models.mobilenet_v2(weights=models.MobileNet_V2_Weights.IMAGENET1K_V1)
model.classifier[1] = nn.Linear(model.last_channel, NUM_CLASSES)

device = "cuda" if torch.cuda.is_available() else "cpu"
model = model.to(device)
crit = nn.CrossEntropyLoss()
opt = torch.optim.Adam(model.parameters(), lr=args.lr)
sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, T_max=args.epochs)


os.makedirs(args.out, exist_ok=True)
best = 0.0
for ep in range(args.epochs):
    model.train()
    tot, corr, loss_sum = 0, 0, 0.0
    for x, y in train_loader:
        x, y = x.to(device), y.to(device)
        opt.zero_grad()
        out = model(x)
        loss = crit(out, y)
        loss.backward()
        opt.step()
        loss_sum += loss.item() * x.size(0)
        corr += (out.argmax(1) == y).sum().item()
        tot += x.size(0)

    sched.step()

    model.eval()
    yt, yp = [], []
    with torch.no_grad():
        for x, y in val_loader:
            x, y = x.to(device), y.to(device)
            pred = model(x).argmax(1)
            yt += y.cpu().tolist()
            yp += pred.cpu().tolist()

    acc = corr / tot
    vcorr = sum(1 for a, b in zip(yt, yp) if a == b)
    vacc = vcorr / max(1, len(yt))
    print(f"ep {ep+1}/{args.epochs}  loss={loss_sum/tot:.4f}  "
          f"train_acc={acc:.3f}  val_acc={vacc:.3f}")

    if vacc > best:
        best = vacc
        torch.save(model.state_dict(), os.path.join(args.out, f"{args.task}_best.pt"))
        print("  -> saved best")
        show_confusion(confusion_matrix(yt, yp, NUM_CLASSES), CLASSES)

print(f"Best val acc = {best:.3f}")
print(f"模型已存：{os.path.join(args.out, args.task + '_best.pt')}")
print("下一步：python eval_model.py --pt <上面这个文件> --flat <同一目录> --task "
      f"{args.task} --size {args.size}")
