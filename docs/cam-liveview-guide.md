# NuttX 摄像头实时显示验证指南（IMX335 → DCMIPP → LTDC）

> Phase 6 成果：在 openvela (NuttX) 中实现 IMX335 摄像头实时显示到 800x480 RGB-LCD。
> 配置序列已在裸机 352_IMX335 例程上逐级验证通过（2026-08-22），再移植到 NuttX。
> 生成日期：2026-08-22 · 生成人：GitHub Copilot

---

## 一、功能说明

| 模块 | 说明 |
|------|------|
| `stm32n6_imx335.c` | IMX335 驱动：上电时序（PWDN=PG6/RST=PG4）、I2C2 读写（16bit 寄存器）、寄存器表、streaming |
| `stm32n6_dcmipp.c` | DCMIPP 驱动：时钟（IC17=300M/IC18=20M）、RIF（CID1+SEC+PRIV）、CSI（2 lane PHY1600）、VC0、PIPE1（RAW10→RGB565 800x480 降采样）、demosaic、**ColorConv 静态 D65 白平衡**、Start/Stop/ISR |
| `apps/examples/cam` | NSH 命令：`cam start` / `cam stop` / `cam status` |
| 显示链路 | IMX335 → MIPI CSI-2 → DCMIPP PIPE1（硬件 ISP 去马赛克 + 白平衡）→ LTDC framebuffer → 7寸 LCD |

**关键点（本版新增）**：DCMIPP PIPE1 **ColorConv 静态白平衡**（D65：R×2.45 / G×1.00 / B×1.55），
替代裸机例程中不可用的 evision AWB 算法，解决画面偏绿问题。

---

## 二、烧录步骤

1. **FSBL**（不变）：`contest2026_294_shiqian/firmware/build/fsbl.hex` @ `0x70000000`
2. **NuttX**（新）：`openvela/cmake_out/atk-dnn647_nsh/nuttx.bin` @ `0x70100000`
   （2026-08-22 22:48 构建，846376 字节，含 ColorConv）

```
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG \
  -w fsbl.hex \
  -w nuttx.bin 0x70100000
```

3. 拨码 **External flash boot**（BOOT1=0, BOOT0=0），上电
4. 串口（PuTTY 115200）应出现 `nsh>` 提示符

---

## 三、验证操作

```nsh
nsh> cam start      # 启动摄像头实时显示（依次：power_on → dcmipp_init → imx335_configure → start_stream → start_capture）
nsh> cam status     # 查看帧计数 / CSI 状态 / 错误
nsh> cam stop       # 停止采集
```

**预期**：
- LCD 显示 IMX335 摄像头实时画面（800x480），**颜色正常**（D65 白平衡，不偏绿）
- `cam status` 帧计数持续增长（~30fps）
- 串口日志无 ERROR

---

## 四、白平衡调节（如需微调）

偏红 → 降低 R 增益；偏绿 → 提高 R/B 增益。修改 `nuttx/arch/arm/src/stm32n6/stm32n6_dcmipp.c`
的 `dcmipp_pipe1_colorconv_config()`：

| 参数 | 当前值（D65） | 说明 |
|------|--------------|------|
| RR（R 增益） | 627（×2.45） | 1.0 = 256 |
| GG（G 增益） | 256（×1.00） | 1.0 = 256 |
| BB（B 增益） | 397（×1.55） | 1.0 = 256 |

---

## 五、注意事项

- **VS Code 缓冲区 vs 磁盘**：`stm32n6_dcmipp.c` 磁盘版本为 21:05 基线 + ColorConv（本版编译依据）。
  VS Code 中该文件如有未保存的实验性修改（如 `dcmipp_pipe1_isp_full_config` Experiment A），
  保存前请与磁盘版本 diff 合并，避免覆盖本版。
- `__enable_irq`：裸机验证发现 FSBL 跳转后中断被禁，但 NuttX 启动流程会自行开启中断（NSH 已正常工作），无需额外处理。
- D-Cache：DCMIPP（写）→ LTDC（读）均为 DMA 路径，不经过 CPU cache，无一致性要求。

---

## 六、相关文件

| 文件 | 说明 |
|------|------|
| `nuttx/arch/arm/src/stm32n6/stm32n6_dcmipp.c/.h` | DCMIPP 驱动（含 ColorConv 白平衡） |
| `nuttx/arch/arm/src/stm32n6/stm32n6_imx335.c/.h` | IMX335 驱动 |
| `nuttx/arch/arm/src/stm32n6/hardware/stm32_dcmipp.h` | DCMIPP 寄存器头 |
| `apps/examples/cam/` | cam 命令应用 |
| `vendor/openvela/boards/atk-dnn647/configs/nsh/defconfig` | CONFIG_STM32_IMX335 / CONFIG_STM32_DCMIPP / CONFIG_EXAMPLES_CAM |
| `SoftwarePackage/baremetal_lrun/` | 裸机验证产物（diag 系列 bin + 验证指南） |
