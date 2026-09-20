# ADR-0004：把 HyperRAM（0x90000000）打通并用于大缓冲

## 状态
已接受

## 背景
内部 RAM 只有 2 MB 可用窗口（`0x34000400`–`0x34200000`，再往上就是 NPU 张量池），
而我们需要：750 KB framebuffer + LVGL 堆 + 相机缓冲 + 模型。

板上有一颗 32 MB HyperRAM，挂在 XSPI1，映射 `0x90000000`，由 BootROM 配好。
先用一次**最小实验**确定了这块内存的适用边界：把 LVGL 堆搬过去
（`CONFIG_LV_MEM_ADR=0x90000000`）会触发 Imprecise data bus error —— 说明当时的
XSPI1 配置不足以支撑随机访问。该实验直接决定了后面的设计：**顺序访问的大缓冲
（相机帧）放 HyperRAM，随机访问频繁的堆留在内部 RAM**。

## 决策
先把 XSPI1 真正配好（驱动只在 `CR.EN` 已置位时早退，导致 BootROM 之后的配置
全部被跳过），再让**相机帧**落到 HyperRAM。

根因是两处事务模板没写：memory-mapped 模式下 `CCR`/`WCCR` 必须先退出
`CR.FMODE` 再写入，且 `DCR2` 的回绕突发要关掉。（详见 `docs/openvela-porting.md`）

定位过程中用到的金标准：原厂裸机工程 **`40_Chinese_Show`** 里的 `MX_XSPI1_Init()`
与 HAL 的 `HAL_XSPI_HyperbusCmd()`（它写的就是 `CCR`/`WCCR` 的模板值），
以及 FSBL 自带的 `bsp/HyperRAM/hyperram.c`。

## 后果
- ✅ HyperRAM 可读写（`eye_cam xspi wr/rd 0x90000000` 自检通过），相机帧落
  `0x90100000`，内部 RAM 只留 framebuffer 与 LVGL 堆。
- ✅ 顺带产出一个**零刷机实验台**：`eye_cam xspi rd/wr/set/dump` 可以在串口上
  直接读写任意寄存器，本项目后来的多个根因都是靠它定位的。
- ⚠️ LVGL 堆仍留在内部 RAM —— 那次实验说明"能读写"不等于"够快/够稳"，
  所以只把**顺序访问的大缓冲**（相机帧）放过去。
