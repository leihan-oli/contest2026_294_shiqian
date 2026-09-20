# verify_onnx.py —— 用 onnxruntime 验证导出的 ONNX 能正常推理
# 用法: python verify_onnx.py models/blink.onnx
#      （不带参数时默认找脚本同级的 models/blink.onnx）
import os
import sys

import numpy as np
import onnxruntime as ort

_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "models", "blink.onnx")
path = sys.argv[1] if len(sys.argv) > 1 else _DEFAULT
sess = ort.InferenceSession(path)
iname = sess.get_inputs()[0].name
ishape = sess.get_inputs()[0].shape
print("输入:", iname, ishape)

# 用输入尺寸构造随机数据
n, c, h, w = [d if isinstance(d, int) else 1 for d in ishape]
x = np.random.randn(n, c, h, w).astype(np.float32)
out = sess.run(None, {iname: x})[0]
print("输出形状:", out.shape)
print("softmax 概率:", np.exp(out[0]) / np.exp(out[0]).sum())
print("预测类别(argmax):", int(out[0].argmax()))
print("ONNX 验证通过!")
