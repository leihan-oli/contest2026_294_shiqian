#!/usr/bin/env bash
# build_sdtest.sh -- standalone SDMMC1 test image (runs after FSBL)
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT="$SCRIPT_DIR/build"
CUBE="$FW/stm32cube"
HALSRC="$CUBE/Drivers/STM32N6xx_HAL_Driver/Src"
CMSIS="$CUBE/Drivers/CMSIS/Device/ST/STM32N6xx/Include"
HALINC="$CUBE/Drivers/STM32N6xx_HAL_Driver/Inc"
CORINC="$CUBE/Drivers/CMSIS/Include"
mkdir -p "$OUT"

export PATH="/home/leihann/openvela/prebuilts/gcc/linux-x86_64/arm-none-eabi/bin:$PATH"
GCC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

COMMON="-mcpu=cortex-m55 -mthumb -mcmse -mfloat-abi=soft -ffunction-sections -fdata-sections -g -O0"
DEFS="-DUSE_HAL_DRIVER -DSTM32N647xx"
INCLUDES="-I$SCRIPT_DIR/Inc -I$HALINC -I$CMSIS -I$CORINC -I$FW/fsbl/Core/Inc -I$FW/cubeide"
WARN="-Wall -Wno-unused-function"

echo "CC  startup"
$GCC $COMMON $DEFS $INCLUDES -c "$FW/cubeide/startup_stm32n647x0hxq_fsbl.s" -o "$OUT/startup.o"

echo "CC  main"
$GCC $COMMON $INCLUDES $DEFS $WARN -c "$SCRIPT_DIR/sdtest_main.c" -o "$OUT/main.o"

echo "CC  system"
$GCC $COMMON $INCLUDES $DEFS $WARN -c "$FW/fsbl/Core/Src/system_stm32n6xx_fsbl.c" -o "$OUT/system.o"

echo "CC  HAL"
for f in stm32n6xx_hal stm32n6xx_hal_rcc stm32n6xx_hal_rcc_ex stm32n6xx_hal_gpio \
         stm32n6xx_hal_sd stm32n6xx_ll_sdmmc stm32n6xx_ll_dlyb \
         stm32n6xx_hal_cortex stm32n6xx_hal_pwr stm32n6xx_hal_pwr_ex; do
  echo "CC  $f"
  $GCC $COMMON $INCLUDES $DEFS $WARN -c "$HALSRC/$f.c" -o "$OUT/$f.o"
done

echo "LD  sdtest.elf"
$GCC $COMMON -T "$SCRIPT_DIR/sdtest.ld" \
  "$OUT/startup.o" "$OUT/main.o" "$OUT/system.o" \
  "$OUT/stm32n6xx_hal.o" "$OUT/stm32n6xx_hal_rcc.o" "$OUT/stm32n6xx_hal_rcc_ex.o" \
  "$OUT/stm32n6xx_hal_gpio.o" "$OUT/stm32n6xx_hal_sd.o" "$OUT/stm32n6xx_ll_sdmmc.o" \
  "$OUT/stm32n6xx_ll_dlyb.o" "$OUT/stm32n6xx_hal_cortex.o" "$OUT/stm32n6xx_hal_pwr.o" \
  "$OUT/stm32n6xx_hal_pwr_ex.o" \
  -o "$OUT/sdtest.elf" -nostdlib -Wl,--gc-sections -Wl,--build-id=none -lgcc -lm

$OBJCOPY -O binary "$OUT/sdtest.elf" "$OUT/sdtest.bin"

echo "=== Outputs ==="
ls -la "$OUT/sdtest.bin" "$OUT/sdtest.elf"
echo "=== Vector check (first 8 bytes should be MSP then Reset) ==="
xxd -l 8 "$OUT/sdtest.bin" 2>/dev/null || od -A x -t x4 -N 8 "$OUT/sdtest.bin"
