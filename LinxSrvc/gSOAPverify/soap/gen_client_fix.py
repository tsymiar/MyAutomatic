#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""由 soapcpp2 生成的 soapClient.cpp 派生 Windows 工程依赖的 soapClient_fix.cpp。

Windows 侧（MFCKline/TestUtils 的 vcxproj）编译的是 fix/soapClient_fix.cpp，
它与 soap/soapClient.cpp 只有书写风格和函数顺序的差别，没有语义差别：
  - 指针/引用紧贴类型：char *msg -> char* msg，struct api__ArrayOfEmp2 &stat -> ...& stat
  - 函数体左花括号独占一行
  - 续行的 || 用 Tab 对齐
  - soap_call_* 统一排到文件末尾，与 fix/ns2_soap_call.h 的声明顺序一致

用法：
    python3 soap/gen_client_fix.py [源文件] [目标文件]
默认源 soap/soapClient.cpp，目标 fix/soapClient_fix.cpp。
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_SRC = os.path.join(HERE, "soapClient.cpp")
DEFAULT_DST = os.path.join(os.path.dirname(HERE), "fix", "soapClient_fix.cpp")

FUNC_HEAD = "SOAP_FMAC5 int SOAP_FMAC6 "
CALL_PREFIX = "soap_call_"
STAMP = "SOAP_SOURCE_STAMP"
TAIL_GUARD = "#if defined(__BORLANDC__)"

# 续行 " <tab>*|| " -> "<tab>|| "
RE_CONTINUE = re.compile(r"^(\t+) \|\| (.*)$")
# 承接上一行右括号的 "\t{ body" -> 上一行补 " {"，body 缩进 +1
RE_HANG_BRACE = re.compile(r"^(\t+)\{(.+)$")
# 函数体首行 "{ body" -> 拆成两行
RE_BODY_BRACE = re.compile(r"^\{(\t+)(.+)$")
# 缩进后多余的一个空格
RE_EXTRA_SPACE = re.compile(r"^(\t+) +(\S.*)$")
# 声明/签名里的 "type *name" -> "type* name"
RE_PTR_DECL = re.compile(r"(\w) ([*&]+)(\w)")
RE_LOCAL_DECL = re.compile(r"^\tstruct \w+ [*&]+\w+;$")


def rewrite(line):
    """单行风格变换，返回 0 行或多行。"""
    m = RE_BODY_BRACE.match(line)
    if m:
        return ["{", m.group(1) + m.group(2)]

    m = RE_CONTINUE.match(line)
    if m:
        return [m.group(1) + "\t|| " + m.group(2)]

    m = RE_HANG_BRACE.match(line)
    if m:
        return ["HANG\t" + m.group(1) + "\t" + m.group(2).lstrip("\t")]

    m = RE_EXTRA_SPACE.match(line)
    if m:
        return [m.group(1) + m.group(2)]

    if line.startswith(FUNC_HEAD) or RE_LOCAL_DECL.match(line):
        return [RE_PTR_DECL.sub(r"\1\2 \3", line)]

    return [line]


def transform(block):
    """变换一个函数块；HANG 标记的行要并回上一行末尾的 " {"。"""
    out = []
    for line in block:
        for item in rewrite(line):
            if item.startswith("HANG\t"):
                if out and out[-1].rstrip().endswith(")"):
                    out[-1] = out[-1] + " {"
                out.append(item[len("HANG\t"):])
                continue
            out.append(item)
    return out


def split_functions(lines):
    """切出函数块，返回 (块列表, 每个块的函数名)。"""
    blocks = []
    names = []
    cur = []
    for line in lines:
        if line.startswith(FUNC_HEAD):
            if cur:
                blocks.append(cur)
            cur = [line]
            names.append(line[len(FUNC_HEAD):].split("(")[0].strip())
            continue
        if cur:
            cur.append(line)
            if line == "}":
                blocks.append(cur)
                cur = []
    if cur:
        blocks.append(cur)
    return blocks, names


def generate(src_text):
    lines = src_text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()

    stamp_idx = next(i for i, l in enumerate(lines) if l.startswith(STAMP))
    tail_idx = max(i for i, l in enumerate(lines) if l == TAIL_GUARD)
    head = lines[:stamp_idx + 1]
    tail = lines[tail_idx:]
    blocks, names = split_functions(lines[stamp_idx + 1:tail_idx])

    plain = []
    calls = []
    for block, name in zip(blocks, names):
        body = transform(block)
        (calls if name.startswith(CALL_PREFIX) else plain).append(body)
    if not calls:
        raise SystemExit("源文件中没有 soap_call_* 函数，拒绝生成")

    out = list(head)
    for body in plain + calls:
        out.append("")
        out.extend(body)
    out.append("")
    out.extend(tail)
    return "\n".join(out) + "\n"


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
    dst = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_DST
    with open(src, "r", encoding="utf-8", errors="ignore") as fh:
        text = fh.read()
    with open(dst, "w", encoding="utf-8", errors="ignore", newline="\n") as fh:
        fh.write(generate(text))
    print("-- 生成: %s" % dst)


if __name__ == "__main__":
    main()
