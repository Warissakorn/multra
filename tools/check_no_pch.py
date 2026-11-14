#!/usr/bin/env python3
"""Compile every translation unit that uses a precompiled header again without it, syntax only.

Windows CI configures with CMAKE_DISABLE_PRECOMPILE_HEADERS=ON, so a file that leans on a PCH for a
full header (for example <nlohmann/json.hpp> where only json_fwd.hpp is included) builds on Linux
and fails only on Windows. Usage: python3 tools/check_no_pch.py build/desktop [--jobs N]
Exit status 1 lists each failing file with its first errors.
"""
import json
import os
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor


def without_pch(command):
    out, skip = [], 0
    for arg in shlex.split(command):
        if skip:
            skip -= 1
            continue
        if arg in ("-include", "-o"):
            skip = 1
            continue
        if arg in ("-c", "-Winvalid-pch"):
            continue
        out.append(arg)
    return out + ["-fsyntax-only"]


def main(argv):
    if len(argv) < 2:
        print(__doc__.strip().splitlines()[-2], file=sys.stderr)
        return 2
    jobs = int(argv[argv.index("--jobs") + 1]) if "--jobs" in argv else os.cpu_count() or 4
    entries = json.load(open(os.path.join(argv[1], "compile_commands.json")))
    units = []
    for e in entries:
        command = e.get("command") or " ".join(shlex.quote(a) for a in e["arguments"])
        if "cmake_pch" in command and "cmake_pch" not in e["file"]:
            units.append((e["directory"], without_pch(command), e["file"]))

    def run(unit):
        result = subprocess.run(unit[1], cwd=unit[0], capture_output=True, text=True)
        return unit[2], result.returncode, result.stderr

    failed = {}
    with ThreadPoolExecutor(jobs) as pool:
        for name, code, err in pool.map(run, units):
            if code:
                failed[name] = [line for line in err.splitlines() if "error" in line][:5]
    print(f"{len(units)} units checked without the PCH; {len(failed)} failed")
    for name in sorted(failed):
        print(name)
        for line in failed[name]:
            print("   ", line[:240])
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
