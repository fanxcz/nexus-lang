#!/usr/bin/env python3
import argparse
import os
import pty
import select
import subprocess
import tempfile
import time

def read_until(master, proc, marker, timeout=10):
    deadline = time.monotonic() + timeout
    data = bytearray()
    while marker not in data and time.monotonic() < deadline:
        ready, _, _ = select.select([master], [], [], 0.1)
        if ready:
            try:
                part = os.read(master, 4096)
            except OSError:
                break
            if not part:
                break
            data.extend(part)
        if proc.poll() is not None and not ready:
            break
    if marker not in data:
        raise AssertionError(f"prompt not received, exit={proc.poll()}, output={bytes(data[-2000:])!r}")
    return bytes(data)

def has_line(data, value):
    return any(line.strip() == value for line in data.replace(bytes([13]), b"").splitlines())

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nexus", required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="nexus-repl-tty-") as directory:
        env = os.environ.copy()
        env["NEXUS_HISTORY_FILE"] = os.path.join(directory, "history.txt")
        master, slave = pty.openpty()
        proc = subprocess.Popen([args.nexus], stdin=slave, stdout=slave, stderr=slave, env=env, close_fds=True)
        os.close(slave)
        try:
            read_until(master, proc, bytes([13, 10]) + b"nexus> ")
            os.write(master, b"1 + 1")
            os.write(master, bytes([27, 91, 72, 27, 91, 67, 27, 91, 67, 27, 91, 67, 27, 91, 67, 127, 32, 13]))
            first = read_until(master, proc, bytes([13, 10]) + b"nexus> ")
            if not has_line(first, b"2"):
                raise AssertionError(f"edited entry did not evaluate: {first!r}")
            os.write(master, bytes([27, 91, 65, 13]))
            second = read_until(master, proc, bytes([13, 10]) + b"nexus> ")
            if not has_line(second, b"2"):
                raise AssertionError(f"history recall did not evaluate: {second!r}")
            os.write(master, bytes([3]))
            read_until(master, proc, bytes([13, 10]) + b"nexus> ")
            os.write(master, b":quit\r")
            proc.wait(timeout=5)
            if proc.returncode != 0:
                raise AssertionError(f"REPL exit code: {proc.returncode}")
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=5)
            os.close(master)
    print("NEXUS REPL TTY editing smoke test: PASS")

if __name__ == "__main__":
    main()
