#!/usr/bin/env python3
# verify_products.py —— ST Edge AI 产物验收（Windows / Linux 通用，零依赖）
#
# 为什么需要它：`network.c` 是 60 万行的生成代码，肉眼只能看文件头。但真正决定
# 能不能上板的是几个**藏在文件深处**的字段，任何一个不对，编译照样过、板子照样
# 启动，只是推理结果错或内存踩踏 —— 属于最难查的一类故障。
#
# 上一次（二分类）就是这样：产物能编译、能跑，但输出全零，查了很久才发现是
# 输出地址与量纲的误会。所以这里把**验收标准写成机器可判的**。
#
# 用法：
#   python verify_products.py <产物目录>
#   python verify_products.py st_ai_output_v20
#
# 退出码：0 = 全过，1 = 有 FAIL

import os
import re
import sys

# --- 本模型（blink 五分类）的既定契约 ---------------------------------------
EXP_IN_W, EXP_IN_H, EXP_IN_C = 128, 64, 3
EXP_IN_LEN = EXP_IN_W * EXP_IN_H * EXP_IN_C      # 24576
EXP_NCLS = 5                                     # closed/open/left/right/other
EXP_POOL = 0x34200000                            # 张量池（与 palm995 同布局）
EXP_WEIGHTS = 0x70400000                         # NOR 权重窗口
EXP_DEV = 16                                     # LL_ATON version dev


class Check:
    def __init__(self):
        self.rows = []

    def add(self, name, ok, got, want):
        self.rows.append((name, ok, str(got), str(want)))

    def report(self):
        width = max(len(r[0]) for r in self.rows)
        nfail = 0
        for name, ok, got, want in self.rows:
            if not ok:
                nfail += 1
            print(f"  {'✅' if ok else '❌'} {name:<{width}}  实测={got:<18} 期望={want}")
        return nfail


def table_entries(src, func_name):
    """从一个 LL_ATON_*_Buffers_Info_Default() 里抽出所有缓冲条目。"""
    m = re.search(re.escape(func_name) + r"\s*\(void\)\s*\{(.*?)\n\}", src, re.S)
    if not m:
        return []
    body = m.group(1)
    out = []
    for part in body.split(".name")[1:]:
        e = {}
        n = re.match(r'\s*=\s*"([^"]*)"', part)
        if not n:
            continue
        e["name"] = n.group(1)
        for key in ("offset_start", "offset_end", "offset_limit",
                    "is_user_allocated", "is_param", "epoch", "nbits"):
            k = re.search(r"\.%s\s*=\s*(-?\d+)" % key, part)
            if k:
                e[key] = int(k.group(1))
        k = re.search(r"\.type\s*=\s*(\w+)", part)
        if k:
            e["type"] = k.group(1)
        k = re.search(r"\.mem_shape\s*=\s*[^\s,]*?([0-9]+(?:_[0-9]+)+)", part)
        if k:
            e["mem_shape"] = k.group(1)
        out.append(e)
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    d = sys.argv[1]
    ncf = os.path.join(d, "network.c")
    ech = os.path.join(d, "network_ecblobs.h")

    if not os.path.isfile(ncf):
        print(f"❌ 找不到 {ncf}")
        return 1

    src = open(ncf, encoding="utf-8", errors="replace").read()
    c = Check()

    # ---- ① 版本 ----------------------------------------------------------
    m = re.search(r'GIT_BRANCH\s+"([^"]+)"', src)
    c.add("GIT_BRANCH 是 STAI-2.0", bool(m and "STAI-2.0" in m.group(1)),
          m.group(1) if m else "缺失", "含 STAI-2.0")

    dev = set(re.findall(r"LL_ATON_VERSION_DEV\s*!=\s*(\d+)", src))
    c.add("LL_ATON_VERSION_DEV", dev == {str(EXP_DEV)},
          ",".join(sorted(dev)) or "缺失", str(EXP_DEV))

    # 4.0 的产物会带这些符号 —— 出现即说明用错了工具链版本
    # 注意：__LL_ATON_LIB_PHYSICAL_TO_VIRTUAL_ADDR 是 2.0 产物里**正常**的
    # cache 维护宏（带 __ 前缀），不能当 4.0 特征用 —— 已实测确认。
    bad = [s for s in ("ec_copy_blob", "ll_aton_rt_user_api") if s in src]
    c.add("无 4.0 专属符号", not bad, ",".join(bad) or "无", "无")

    # ---- ② 输入张量 -------------------------------------------------------
    # 生成代码在 LL_ATON_DBG_BUFFER_INFO_EXCLUDED==0 时会把权重(194)和量化参数
    # (88) 一起列进同一张表，全部带 .is_param = 1 —— 只按名字过滤不干净
    # （量化参数叫 Conv2D_8_mul_scale_17，不含 weight/bias）。实测 is_param 才是
    # 可靠判据：is_param==0 的输入/输出各恰好一条。
    def is_tensor(e):
        return e.get("is_param") == 0

    inp = [e for e in table_entries(src, "LL_ATON_Input_Buffers_Info_Default")
           if is_tensor(e)]
    real = inp
    if real:
        e = real[0]
        c.add("输入 张量数", len(real) == 1, len(real), 1)
        c.add("输入 is_user_allocated", e.get("is_user_allocated") == 1,
              e.get("is_user_allocated"), 1)
        c.add("输入 type", e.get("type") == "DataType_INT8", e.get("type"),
              "DataType_INT8")
        ln = e.get("offset_end", 0) - e.get("offset_start", 0)
        c.add("输入 字节数", ln == EXP_IN_LEN, ln, EXP_IN_LEN)
        ms = e.get("mem_shape", "")
        c.add("输入 mem_shape (NCHW)",
              ms in ("%d_%d_%d_%d" % (1, EXP_IN_C, EXP_IN_H, EXP_IN_W),
                     "%d_%d_%d_%d" % (1, EXP_IN_C, EXP_IN_W, EXP_IN_H)),
              ms, "1_3_64_128")
    else:
        c.add("输入张量表可解析", False, "解析失败", "1 个条目")

    # ---- ③ 输出张量（五分类的关键）---------------------------------------
    out = [e for e in table_entries(src, "LL_ATON_Output_Buffers_Info_Default")
           if is_tensor(e)]
    rout = out
    if rout:
        e = rout[0]
        c.add("输出 张量数", len(rout) == 1, len(rout), 1)
        n = e.get("offset_end", 0) - e.get("offset_start", 0)
        c.add("输出 类别数", n == EXP_NCLS, n, EXP_NCLS)
        c.add("输出 type", e.get("type") == "DataType_INT8", e.get("type"),
              "DataType_INT8")
        c.add("输出 mem_shape", e.get("mem_shape") == "1_%d" % EXP_NCLS,
              e.get("mem_shape"), "1_%d" % EXP_NCLS)
        c.add("输出来自最后一个 epoch",
              e.get("epoch") is not None and e.get("epoch") > 1,
              e.get("epoch"), "> 1")
    else:
        c.add("输出张量表可解析", False, "解析失败", "1 个条目")

    # ---- ④ 内存布局 -------------------------------------------------------
    bases = set(int(x, 16) for x in
                re.findall(r"\(unsigned char \*\)\((0x[0-9a-fA-F]+)UL\)", src))
    c.add("张量池基址", EXP_POOL in bases,
          ",".join(hex(b) for b in sorted(bases)[:4]) or "无", hex(EXP_POOL))
    c.add("NOR 权重地址", EXP_WEIGHTS in bases,
          hex(EXP_WEIGHTS) in [hex(b) for b in bases], hex(EXP_WEIGHTS))

    # ---- ⑤ 文件齐备 -------------------------------------------------------
    c.add("network_ecblobs.h 存在", os.path.isfile(ech),
          os.path.isfile(ech), True)

    raws = [f for f in os.listdir(d) if f.endswith(".raw")]
    c.add("权重 .raw 存在", len(raws) == 1, ",".join(raws) or "无", "1 个")
    if len(raws) == 1:
        sz = os.path.getsize(os.path.join(d, raws[0]))
        c.add("权重 .raw 大小合理", 0 < sz < 12 * 1024 * 1024,
              f"{sz:,} B", "< 12MB")

    print(f"=== 产物验收: {d} ===")
    print()
    nfail = c.report()
    print()
    if nfail:
        print(f"❌ 有 {nfail} 项未通过 —— 先别回传，把上面的实测值发回给我")
    else:
        print("✅ 全部通过 —— 可以打包回传")
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
