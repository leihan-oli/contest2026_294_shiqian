# quickapp — 本目录为何是空的

模板为快应用 demo 预留了本目录。**本作品不是快应用**，而是 openvela（NuttX）
上的**原生 C 应用**（`app/examples/eye_cam/`），因此这里没有内容。

理由很直接：作品要直接驱动 DCMIPP 摄像头、NPU（ATON）与 LTDC 屏幕，
并在同一块 framebuffer 上与 LVGL 互斥协作 —— 这些都在应用层之下，
快应用容器不提供对应通路。

参赛方向也不冲突：大赛列出"AI 硬件产品创新 / 手表应用创新 / 新硬件平台适配"
三类，本作品属于第一类（STM32N647 + NPU 的端侧眼控交互）。
