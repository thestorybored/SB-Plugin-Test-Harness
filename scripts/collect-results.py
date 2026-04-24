#!/usr/bin/env python3
"""Wrap plugin-harness and synthesize a JUnit failure on abnormal exit.

If plugin-harness writes its own --junit-out file successfully (exit 0/1), this
script is a near-no-op (it just streams output). If the binary segfaults or is
killed by a signal, it captures the stderr tail and writes a fresh JUnit XML
with a single failed <testcase> describing the crash.

Usage:
    collect-results.py --junit-out OUT.xml -- <plugin-harness args...>
"""
from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
import time
import xml.sax.saxutils as xs
from pathlib import Path


def parse_args(argv: list[str]) -> tuple[Path, list[str]]:
    if "--" not in argv:
        sys.exit("collect-results.py: expected '--' separator before harness args")
    sep = argv.index("--")
    own = argv[:sep]
    fwd = argv[sep + 1:]
    p = argparse.ArgumentParser()
    p.add_argument("--junit-out", required=True, type=Path)
    p.add_argument("--scenario-name", default="harness")
    a = p.parse_args(own)
    return a.junit_out, fwd


def write_crash_junit(out: Path, scenario: str, exit_code: int,
                      duration_secs: float, stderr_tail: str) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    msg = f"plugin-harness exited abnormally (code={exit_code}"
    if exit_code < 0:
        msg += f", signal={-exit_code} {signal.Signals(-exit_code).name}"
    elif exit_code > 128:
        sig = exit_code - 128
        try:
            msg += f", signal={sig} {signal.Signals(sig).name}"
        except ValueError:
            pass
    msg += ")"
    xml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<testsuites>\n'
        f'  <testsuite name="plugin-harness" tests="1" failures="1" errors="0"'
        f' time="{duration_secs:.6f}">\n'
        f'    <testcase classname="harness" name="{xs.escape(scenario)}.crash"'
        f' time="{duration_secs:.6f}">\n'
        f'      <failure message="{xs.escape(msg)}"/>\n'
        f'      <system-err>{xs.escape(stderr_tail)}</system-err>\n'
        f'    </testcase>\n'
        f'  </testsuite>\n'
        f'</testsuites>\n'
    )
    out.write_text(xml, encoding="utf-8")


def main(argv: list[str]) -> int:
    junit_out, fwd = parse_args(argv)

    # Determine scenario name from forwarded args (best-effort).
    scenario = "unknown"
    for i, a in enumerate(fwd):
        if a == "--scenario" and i + 1 < len(fwd):
            scenario = fwd[i + 1]; break
        if a.startswith("--scenario="):
            scenario = a.split("=", 1)[1]; break

    # Forward --junit-out to the binary too if not already passed.
    if "--junit-out" not in fwd and not any(a.startswith("--junit-out=") for a in fwd):
        fwd = fwd + ["--junit-out", str(junit_out)]

    start = time.monotonic()
    try:
        proc = subprocess.Popen(fwd, stdout=sys.stdout,
                                stderr=subprocess.PIPE, text=True)
    except FileNotFoundError as e:
        write_crash_junit(junit_out, scenario, 127, 0.0, str(e))
        print(str(e), file=sys.stderr)
        return 127

    stderr_lines: list[str] = []
    assert proc.stderr is not None
    for line in proc.stderr:
        sys.stderr.write(line)
        sys.stderr.flush()
        stderr_lines.append(line)
        if len(stderr_lines) > 200:
            stderr_lines.pop(0)
    rc = proc.wait()
    elapsed = time.monotonic() - start

    crashed = rc < 0 or rc >= 128
    if crashed or not junit_out.exists():
        write_crash_junit(junit_out, scenario, rc, elapsed, "".join(stderr_lines))
    return rc if rc >= 0 else 128 + (-rc)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
