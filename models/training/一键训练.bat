@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ============================================================
echo   眼动 blink 模型训练（五分类: closed/open/left/right/other）
echo   数据: eye_photo\   输出: models\
echo ============================================================
echo.

echo [1/5] 训练
python train_blink.py --flat eye_photo --task blink --size 64x128
if errorlevel 1 goto :fail

echo.
echo [2/5] 验收（通过线 0.90）—— 未达标【不中止】
echo       为什么不停：这一轮的产物要用来出混淆矩阵，未达标也有诊断价值。
echo       未达标时不要下调通过线，把混淆矩阵连同产物一起回传即可。
python eval_model.py --pt models\blink_best.pt --flat eye_photo --task blink --size 64x128
if errorlevel 1 echo   ^^^^ 验收未达 0.90，但继续生成产物（见上一行说明）

echo.
echo [3/5] 导出 ONNX
python export_onnx.py --pt models\blink_best.pt --out models\blink.onnx --task blink --size 64x128
if errorlevel 1 goto :fail

echo.
echo [4/5] 验证 ONNX
python verify_onnx.py models\blink.onnx
if errorlevel 1 goto :fail

echo.
echo [5/5] 量化 int8（上板必需，不量化 NPU 基本用不上）
python quantize_onnx.py --onnx models\blink.onnx --flat eye_photo --task blink --size 64x128
if errorlevel 1 goto :fail

echo.
echo ============================================================
echo   训练阶段全部完成！产物: models\blink_best.pt  models\blink.onnx  models\blink_qdq_int8.onnx
echo   下一步: 用 ST Edge AI 转 STM32N6 模型（用 blink_qdq_int8.onnx！见 Windows训练指南.md 第6节）
echo           !! ST Edge AI 必须是 2.0 版（v2.0.0-20049），不能用 4.0 !!
echo           生成完用 python verify_products.py ^<产物目录^> 自检，全过再回传
echo ============================================================
pause
exit /b 0

:fail
echo.
echo ============================================================
echo   出错了。请看上方红字，或查 Windows训练指南.md §4 常见问题
echo ============================================================
pause
exit /b 1
