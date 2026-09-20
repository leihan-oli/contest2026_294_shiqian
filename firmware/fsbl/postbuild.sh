#!/bin/bash
#
# FSBL post-build step (fixed):
#   - Pad 0x1C0 (448) zero bytes in front of the FSBL bin so the signing
#     tool (-t fsbl) reads the REAL Reset_Handler at input offset 0x1C4 and
#     stamps the correct entry (header 0x70).  The prebuilt XIP FSBL shipped
#     with the vector table at image offset 0x1C0; without the same padding
#     the LRUN FSBL's header entry becomes a Default_Handler address and
#     flash boot dead-loops.
#   - Sign with STM32_SigningTool_CLI and emit Binary/fsbl.hex at 0x70000000.
#
# Invoked from CubeIDE as:  bash ../postbuild.sh "${cubeide_cubeprogrammer_path}"

ProjectDir="$(dirname "$(readlink -f "$0")")"
STM32SigningTool="$1/STM32_SigningTool_CLI"
echo "[postbuild] ProjectDir=$ProjectDir"
echo "[postbuild] SigningTool=$STM32SigningTool"

# --- pick the NEWEST *_FSBL.bin from the build output dirs ---
ProjectOut="$(ls -t "$ProjectDir"/Release/*_FSBL.bin "$ProjectDir"/Debug/*_FSBL.bin 2>/dev/null | head -1)"
if [ -z "$ProjectOut" ]; then
    echo "[postbuild] ERROR: no *_FSBL.bin found under $ProjectDir (Release/Debug)"
    exit 1
fi
echo "[postbuild] bin: $ProjectOut ($(stat -c%s "$ProjectOut") bytes)"

# --- pad 0x1C0 zero bytes in front of the bin ---
Padded="${ProjectOut%.*}-padded.${ProjectOut##*.}"
BinSize=$(stat -c%s "$ProjectOut")
{
    # 448 NUL bytes (robust; works in Git Bash / MSYS2 / Linux, no /dev/zero)
    printf '%448s' '' | tr ' ' '\000'
    cat "$ProjectOut"
} > "$Padded"
PaddedSize=$(stat -c%s "$Padded")
echo "[postbuild] padded: $PaddedSize bytes (expect $((BinSize + 448)))"
if [ "$PaddedSize" -ne "$((BinSize + 448))" ]; then
    echo "[postbuild] ERROR: padding failed, size mismatch"
    rm -f "$Padded"
    exit 1
fi

# --- sign ---
ProjectConv="${ProjectOut%.*}-trusted.${ProjectOut##*.}"
rm -f "$ProjectConv"
"$STM32SigningTool" -bin "$Padded" -nk -of 0x80000000 -t fsbl -o "$ProjectConv" -hv 2.3 -dump "$ProjectConv" || {
    echo "[postbuild] ERROR: signing failed"
    rm -f "$Padded"
    exit 1
}
rm -f "$Padded"

# --- emit hex at 0x70000000 ---
arm-none-eabi-objcopy -I binary "$ProjectConv" --change-addresses 0x70000000 -O ihex "$ProjectDir"/../../Binary/fsbl.hex || {
    echo "[postbuild] ERROR: objcopy failed"
    exit 1
}
echo "[postbuild] OK: hex regenerated from $ProjectOut"
echo "[postbuild] verify: file 0x240-0x400 must be all 0x00, header 0x70 = vector-table Reset"
