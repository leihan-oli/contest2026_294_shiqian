#!/bin/bash
# =============================================================================
# Linux-side FSBL build for STM32N647 flash-boot bring-up (self-contained).
#
# This is a SELF-CONTAINED version of the original bring-up build script:
#   - All ST/board resources live inside this repo (firmware/):
#       firmware/fsbl        : FSBL Core sources (main.c, extmem, hal_msp, ...)
#       firmware/stm32cube   : STM32Cube FW N6 (HAL driver + CMSIS + ExtMem_Manager)
#       firmware/bsp         : 正点原子 BSP (HyperRAM + NORFlash)
#       firmware/cubeide     : startup asm + linker script
#   - Paths are derived from the script location, so the script can be run
#     from anywhere and works with only this repository checked out.
#
# Usage:
#   firmware/scripts/build_fsbl.sh
#
# Output:
#   <repo>/firmware/build/fsbl.elf  and  fsbl.bin   (see OUT below)
#   Convert to a bootable hex with: firmware/scripts/gen_fsbl_hex.py
#
# Reproduces the CubeIDE Debug build (LRUN boot mode + RIF/RISAF config).
# =============================================================================
set -e

# --- Locate repo root from script location --------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE="$(cd "$SCRIPT_DIR/.." && pwd)"          # .../contest2026_294_shiqian/firmware

PROJ="$BASE/fsbl"
FW="$BASE/stm32cube"
BSP="$BASE/bsp"
CUBEIDE="$BASE/cubeide"
OUT="$BASE/build"
rm -rf "$OUT"; mkdir -p "$OUT"

# --- Toolchain ------------------------------------------------------------
# Prefer the openvela prebuilt ARM GCC if present, else fall back to PATH.
OPENVELA_GCC=/home/leihann/openvela/prebuilts/gcc/linux-x86_64/arm-none-eabi/bin/arm-none-eabi-gcc
if [ -x "$OPENVELA_GCC" ]; then
  GCC="$OPENVELA_GCC"
elif command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  GCC=$(command -v arm-none-eabi-gcc)
else
  echo "ERROR: arm-none-eabi-gcc not found (set GCC= or add to PATH)"; exit 1
fi

COMMON="-mcpu=cortex-m55 -mthumb -mcmse -Os -g3 -ffunction-sections -fdata-sections -fno-common"
DEFS="-DSTM32N647xx -DUSE_HAL_DRIVER"
INC="-I $PROJ/Core/Inc \
     -I $BSP/HyperRAM \
     -I $BSP/NORFlash \
     -I $FW/Drivers/STM32N6xx_HAL_Driver/Inc \
     -I $FW/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy \
     -I $FW/Drivers/CMSIS/Device/ST/STM32N6xx/Include \
     -I $FW/Drivers/CMSIS/Include \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/boot \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/sal \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/nor_sfdp \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/psram \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/sdcard \
     -I $FW/Middlewares/ST/STM32_ExtMem_Manager/user"

compile_c() {
  local src="$1"
  local obj="$OUT/$(basename "${src%.c}").o"
  echo "CC  $src"
  "$GCC" $COMMON $DEFS $INC -c "$src" -o "$obj"
}

# --- Core sources (this repo: firmware/fsbl) ------------------------------
for f in main extmem_manager stm32n6xx_hal_msp system_stm32n6xx_fsbl stm32n6xx_it; do
  compile_c "$PROJ/Core/Src/$f.c"
done

# --- BSP sources (firmware/bsp) -------------------------------------------
for f in hyperram norflash norflash_mx25um25645g norflash_by25fq128el norflash_xspi extmem_user_driver; do
  compile_c "$BSP/HyperRAM/$f.c" 2>/dev/null || compile_c "$BSP/NORFlash/$f.c"
done

# --- Middleware sources (firmware/stm32cube/Middlewares) -------------------
for f in stm32_extmem; do
  compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/$f.c"
done
compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/boot/stm32_boot_lrun.c"
# stm32_boot_xip.c is NOT built: the FSBL selects LRUN boot mode
# (stm32_extmem_conf.h includes stm32_boot_lrun.h; XIP conflicts).
for f in stm32_sal_xspi stm32_sal_sd; do
  compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/sal/$f.c"
done
for f in stm32_sfdp_driver stm32_sfdp_data; do
  compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/nor_sfdp/$f.c"
done
compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/psram/stm32_psram_driver.c"
compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/sdcard/stm32_sdcard_driver.c"
compile_c "$FW/Middlewares/ST/STM32_ExtMem_Manager/user/stm32_user_driver.c"

# --- HAL driver sources (modules enabled in stm32n6xx_hal_conf.h) ----------
for f in stm32n6xx_hal stm32n6xx_hal_rcc stm32n6xx_hal_rcc_ex stm32n6xx_hal_gpio \
         stm32n6xx_hal_rif stm32n6xx_hal_dma stm32n6xx_hal_cortex \
         stm32n6xx_hal_xspi stm32n6xx_hal_pwr stm32n6xx_hal_pwr_ex; do
  compile_c "$FW/Drivers/STM32N6xx_HAL_Driver/Src/$f.c"
done

# --- Startup asm + link (firmware/cubeide) --------------------------------
echo "AS  startup"
"$GCC" $COMMON -c "$CUBEIDE/startup_stm32n647x0hxq_fsbl.s" -o "$OUT/startup.o"

echo "LD  fsbl.elf"
"$GCC" $COMMON -T "$CUBEIDE/STM32N647X0HXQ_AXISRAM2_fsbl.ld" \
  -Wl,--gc-sections -Wl,--cmse-implib -Wl,--out-implib="$OUT/secure_nsclib.o" \
  -Wl,--build-id=none \
  -Wl,--Map="$OUT/fsbl.map" -o "$OUT/fsbl.elf" \
  "$OUT"/*.o -lc -lm -lnosys

echo "=== BUILD OK ==="
ls -la "$OUT/fsbl.elf"

# ===========================================================================
# bin extraction - CRITICAL: use FULL objcopy (NO -j filters!)
#
# Do NOT use:  objcopy -O binary -j .isr_vector -j .text -j .data
# That drops .ARM.exidx / .fini_array / .init_array (0x341880D4..0x341880E8)
# which contain function pointers. The resulting hex has zeros there and
# the FSBL faults at startup (LEDs all dark = confirmed on hardware).
#
# Full objcopy works because .data has LMA=0x341880E8 (AT> ROM in the
# linker script), so the output stays contiguous and matches the
# original CubeIDE-produced image byte-for-byte.
# ===========================================================================
OBJCOPY=${GCC%gcc}objcopy
"$OBJCOPY" -O binary "$OUT/fsbl.elf" "$OUT/fsbl.bin"
echo "=== BIN extracted: $OUT/fsbl.bin ($(stat -c%s "$OUT/fsbl.bin") bytes) ==="

# --- Bootable hex (STM32 FSBL header + 0x1C0 pad + base 0x70000000) ----------
echo "=== HEX generated ==="
python3 "$SCRIPT_DIR/gen_fsbl_hex.py" "$OUT/fsbl.bin" "$OUT/fsbl.hex"
echo "=== Done: flash $OUT/fsbl.hex @ 0x70000000 ==="
