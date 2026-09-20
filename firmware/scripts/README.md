# firmware/scripts — 构建与 hex 验证脚本

Linux 侧完整构建 FSBL 与生成/验证 hex 的工具链（全部原创）。

## 构建

| 脚本 | 用途 | 用法 |
|------|------|------|
| `build_fsbl.sh` | Linux 侧构建自定义 FSBL | `./build_fsbl.sh` → 产出 `fsbl_build/fsbl.bin` |

关键点（踩坑教训，详见 docs）：

- `arm-none-eabi-gcc` 必须加 **`-mcmse`**（触发 `CPU_IN_SECURE_STATE`，启用安全别名 + RIF API）
- 必须加 **`-Wl,--build-id=none`**（否则 `.note.gnu.build-id` 顶掉向量表）
- **objcopy 提取 bin 禁止 `-j` 过滤**（排除 `.ARM.exidx/.fini_array` 会全灭）
- 只编译 LRUN（`stm32_boot_lrun.c`），`stm32_boot_xip.c` 会冲突

## hex 生成与验证

| 脚本 | 用途 |
|------|------|
| `gen_fsbl_hex.py` | 从 bin 生成完整 hex（`0x6C`=0x1C0+实际 bin 长度、`0x70`=Reset、`0x64`=byte_sum，base=`0x70000000`） |
| `make_padded_v2.py` | 自动检测向量表 → 加 `0x1C0` 零填充 → 修正 header 字段 |
| `build_src_hex.py` | 复用旧 header + 新 bin 构造源 hex |
| `rebase_hex.py` | hex 基址重定位（规范用 `0x70000000`） |

## 验证/分析

| 脚本 | 用途 |
|------|------|
| `analyze_fsbl_header.py` | 检查 FSBL header（填充是否全零、`0x70`/`0x6C`/`0x64` 是否正确） |
| `compare_hex.py` | 对比两个 hex（header 字段 + 填充 + byte_sum + 代码 diff） |

## 已烧录固件对应脚本链

`fsbl_rifsecfix.hex` = `build_fsbl.sh` → bin → `gen_fsbl_hex.py`（或 `build_src_hex.py` + `make_padded_v2.py`）

> ⚠️ 每次重建后必须用 `analyze_fsbl_header.py` 验证：`0x240-0x400` 全零、`0x70`=Reset、`0x6C`=0x1C0+bin 长度。
