# export_onnx.py —— 把 .pt 转 ONNX (ST Edge AI 需要 ONNX)
#
# 2026-09-15 改版：支持非正方形输入（如 64x128，需与 train_blink.py 的 --size
# 完全一致），并改用 --task 取代旧版的 --gaze 开关。
#
# 用法:
#   python export_onnx.py --pt D:\eye_ai\models\blink_best.pt \
#          --out D:\eye_ai\models\blink.onnx --task blink --size 64x128
#   python export_onnx.py --pt D:\eye_ai\models\gaze_best.pt \
#          --out D:\eye_ai\models\gaze.onnx  --task gaze  --size 64x128
#
# 导出后自检：python verify_onnx.py D:\eye_ai\models\blink.onnx

import argparse
import os

import torch
from torch import nn
from torchvision import models

from eye_common import parse_size, task_classes

parser = argparse.ArgumentParser()
parser.add_argument("--pt", required=True)
parser.add_argument("--out", required=True)
parser.add_argument("--task", choices=["blink", "gaze"], default="blink")
parser.add_argument("--size", default="64x128",
                    help="'高x宽'（如 64x128）或单个整数表示正方形")
args = parser.parse_args()

CLASSES = task_classes(args.task)
NUM_CLASSES = len(CLASSES)
H, W = parse_size(args.size)

model = models.mobilenet_v2()
model.classifier[1] = nn.Linear(model.last_channel, NUM_CLASSES)
model.load_state_dict(torch.load(args.pt, map_location="cpu"))
model.eval()

x = torch.randn(1, 3, H, W)
torch.onnx.export(model, x, args.out,
                  input_names=["input"], output_names=["output"],
                  opset_version=17, dynamic_axes=None)
print(f"ONNX saved -> {args.out}")
print(f"  task    = {args.task}")
print(f"  classes = {CLASSES}   (输出顺序必须与板端一致)")
print(f"  input   = 1x3x{H}x{W}  (NCHW，注意这里是 高x宽)")
print("  下一步:")
print(f"    stedgeai generate --target stm32n6 "
      f"--st-neural-art default@user_neuralart_eye.json --model {os.path.basename(args.out)}")
