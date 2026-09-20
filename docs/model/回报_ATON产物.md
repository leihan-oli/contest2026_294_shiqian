# 回报 Linux Agent：眼动模型 v3 ATON 产物生成完成（ST Edge AI Core 2.0）

- 日期：2026-09-19
- 对应：`交付包_Linux验收v3_给Windows_2026-09-19` 的 §5 指示（放行 stedgeai generate）
- 状态：✅ 生成完成、`verify_products.py` **18/18 全绿**、**四量化常量与你们给值逐项一致**（§3）
- 交付包：`交付包_眼动模型v3_ATON产物_给Linux_2026-09-19/`

---

## 0. 一句话结论

用 `blink_v3_qdq_int8.onnx` 在 **ST Edge AI Core v2.0.0-20049** 上生成成功
（**77.5 s、0 warning**）：epochs **57、软件实现 0**（全部硬件/EC）；
权重 2.765 MB @ `0x70400000`、激活 240 kB @ npuRAM3、hyperRAM 0；
`_Default` 符号族（未加 `--network-name`）。
**§5.3 四常量核对：全部一致，板端常量可直接按 §3 左列更新。**

## 1. 生成参数

| 项 | 值 |
|---|---|
| 工具 | ST Edge AI Core **v2.0.0-20049**（ISPU 1.1.0 / MLC 1.1.0 / StellarStudioAI 2.0.0 / STM32CubeAI 10.0.0） |
| 模型 | `blink_v3_qdq_int8.onnx`（model_hash `0x3e031ee377baccbb963392192653521f`） |
| 命令 | 见 `03_生成配置/actual_commands.txt`（`--no-inputs-allocation`；不加 `--network-name`） |
| 耗时 | **77.5 s**，0 warning |
| 选项 | allocate-outputs（输出由工具分配） |
| 配置 | json/mpool 一字未改（json sha256 `673cd357…`、mpool sha256 `19d1c435…`） |

## 2. 产物（`01_模型产物/`）

| 文件 | 字节 | 说明 |
|---|---|---|
| **network.c** | 637,038 | 含 `_Default` 符号族 |
| **network_ecblobs.h** | 497,832 | |
| **network_atonbuf.xSPI2.raw** | 2,899,745 | 权重 blob → NOR `0x70400000`（范围 `0x70400000–0x706C3F30`） |
| network_generate_report.txt | 68,843 | 工具报告全文 |
| network_c_info.json | 1,364,738 | 网络信息（含全张量量化参数） |
| blink_v3_qdq_int8_OE_3_1_0.onnx | 8,904,957 | 中间 ONNX（OE 2.0 原生） |
| blink_v3_qdq_int8_OE_3_1_0_Q.json | 2,750,338 | 中间量化 JSON |
| LICENSE.txt | 10,648 | |

**对照值**：`network_atonbuf.xSPI2.raw` **md5 = `fbc1f4c016143f3c48fdb78a25dcedb1`**
（供烧录/板端对照；v20=571880ba…、v21=671b11c3…，三代互不相同——本轮为全新值）

**源头核对**：送入模型 md5 = `3e031ee377baccbb963392192653521f`，
与你们验收报告中读出的 `blink_v3_qdq_int8.onnx` md5 **逐位一致**
（生成日志 model_hash 同值）；旧模型（`blink_qdq_calib345.onnx`/`blink_qdq_int8.onnx`）
md5 均不同，未混用。

关键运行参数（对照 v20/v21）：

- **epochs 57，其中 SW = 0**（全部硬件 EC）——与 v20/v21 同构
- 输入 `Input_0_out_0`：int8 `1×3×64×128`，**用户分配 24,576 B（32B 对齐）**
- 激活：**240 kB @ npuRAM3（0x34200000）**；npuRAM4/5/6 零占用
- 权重：2.765 MB @ octoFlash `0x70400000–0x706C3F30`
- hyperRAM：**0**
- 总计：3.023 MB（weights 2.765 MB + activations 264 kB）
- MACC：53,577,145

## 3. §5.3 四常量核对（network.c 实测 → 逐项比对）

| 常量 | **network.c 实测（本包产物）** | **你们给的（ONNX 源头）** | 判定 |
|---|---|---|---|
| input scale | `0.00707420241087675`（L172） | 0.0070742024 | ✅ |
| input zero_point | `-14`（L173） | −14 | ✅ |
| output scale | `0.170157581567764`（L4817 / L5013） | 0.17015758 | ✅ |
| output zero_point | `-34`（L4818 / L5014） | −34 | ✅ |

- 行号证据全文：`02_验收记录/quant_constants_check.txt`
  （输入 buf 使用处 L462-463；输出 buf 使用处 L4841-4842）
- 生成摘要亦直接打印：`QLinear(0.007074202,-14,int8)` / `QLinear(0.170157582,-34,int8)`

**结论：与 ONNX 源头一致，无转换偏差。** 板端 `EYE_CAM_IN_SCALE / EYE_CAM_IN_ZP /
EYE_CAM_OUT_SCALE / EYE_CAM_OUT_ZP`（及 `Q_GAIN/Q_BIAS` 派生）可直接按上表左列更新。

**⚠️ 提醒沿用**：output_scale 相对 v2 时代（0.129406050）变化 **+31.5%**，别忘同步板端代码。

## 4. 自检（`02_验收记录/verify_products_output.txt`）

`verify_products.py st_ai_output_v3` → **18/18 全绿**：STAI-2.0 / VERSION_DEV=16 /
无 4.0 符号 / 输入 `1×3×64×128` int8 24576B user_allocated / 输出 5 类 int8 `1_5`
（来自第 57 epoch）/ 池 `0x34200000` + 权重 `0x70400000` / ecblobs & raw 齐备。

## 5. 已知事项（沿用你们的结论）

- PC 端 int8 全图误判数差异（我们 1 / 你们 7）系 ORT 版本差异；**板端以实测为准**。
- 板端测试请加一段 **"远距离向左看"**（dx<94 档量化敏感样本）。
- 落板两处移植（`ECBLOB_RUNTIME_SECTION → .nsblob`、`aton_model_ecblobs_to_ns()` 钩子）
  由 Linux 侧脚本处理，本包未做——与你们的说明一致。

## 6. 校验

`CHECKSUMS.sha256`（LF、UTF-8 无 BOM，`sha256sum -c` 可验）。
