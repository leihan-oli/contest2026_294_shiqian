#!/usr/bin/env python3
"""Re-apply the two board-side edits to the freshly generated network.c.

The generator has no idea this board puts the epoch program in a non-secure
RAM section, nor that the driver expects an ecblobs hook, so both have to be
re-added by hand every time the model is regenerated.  They were lifted
verbatim from the version that was working, so the result should differ from
the package copy by exactly these lines and nothing else.
"""
import sys

P = "/home/leihann/openvela/nuttx/libs/ai_aton/models/eye/network.c"
s = open(P, encoding="utf-8").read()
before = len(s)

MACRO = '''/* The epoch program buffer is a runtime-filled RAM array.  Put it in the
 * "nsblob" section, which the board linker script places in the non-secure
 * alias window: the NPU is a non-secure master, so its program buffers must
 * live in a non-secure RISAF region, and the CPU reaches that region through
 * the non-secure alias (a secure alias access would be rejected by the
 * non-secure region).  Without this the blob lands in secure SRAM and the NPU
 * cannot fetch its program.
 */

#define ECBLOB_RUNTIME_SECTION __attribute__((section(".nsblob")))
#include "network_ecblobs.h"
'''

a1 = '#include "network_ecblobs.h"\n'
n = s.count(a1)
if n != 1:
    sys.exit("FAIL anchor1: found %d" % n)
s = s.replace(a1, MACRO, 1)
print("  ok  macro inserted before the include")

HOOK = '''
/* Publish the secondary epoch blobs in non-secure RAM (called by the driver
 * before the model is started); returns a checksum of the copies.
 *
 * 本模型的 epoch 程序是 100% 硬件执行（software epochs = 0），生成器因此只产出
 * 一个 blob（_ec_blob_1），没有需要二次搬运的 secondary blob —— 注意
 * network_ecblobs.h 里也不存在 aton_ecblobs_copy_to_ns() / ATON_NS_BLOB_BASE，
 * 这与 pose994/palm995（含软件 fallback、blob 有多个）不同。
 *
 * 该唯一 blob 通过上面的 ECBLOB_RUNTIME_SECTION 直接落在非安全窗口（.nsblob
 * 段），由 LL_ATON_EC_Network_Init_Default() 用 ec_copy_program() 就地填充。
 * 保留本钩子是为了让驱动（stm32n6_aton_aie.c）的启动/清除日志路径保持完整：
 * 它没有拷贝动作，故校验和为 0。
 */

uint32_t aton_model_ecblobs_to_ns(void)
{
  return 0;
}
'''

a2 = "  return buff_info;\n}\n"
# Three functions return this struct, so take the last one and insist that
# nothing but whitespace follows it - that is the one at the end of the file,
# which is where the hook has to go.
i = s.rfind(a2)
if i < 0:
    sys.exit("FAIL anchor2: not found")
tail = s[i + len(a2):]
if tail.strip():
    sys.exit("FAIL anchor2: found, but not at end of file (tail=%r)"
             % tail[:60])
s = s[:i + len(a2)] + HOOK + tail
print("  ok  hook appended at end of file")

assert len(s) > before
open(P, "w", encoding="utf-8").write(s)

# Read back from disk: a successful write is not proof of what landed.
back = open(P, encoding="utf-8").read()
assert back == s, "read-back mismatch"
assert back.count("ECBLOB_RUNTIME_SECTION") == 2, \
    back.count("ECBLOB_RUNTIME_SECTION")        # the #define and its comment
assert back.count("aton_model_ecblobs_to_ns") == 1
print("  ok  verified on disk: %d -> %d bytes, %d lines"
      % (before, len(back), back.count("\n") + 1))
