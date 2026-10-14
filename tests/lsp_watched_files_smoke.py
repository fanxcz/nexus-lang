#!/usr/bin/env python3
"""Exercise live LSP workspace cache invalidation on edits, renames and deletes."""
from __future__ import annotations

import argparse
import json
import queue
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any, BinaryIO


def read_message(stream: BinaryIO) -> dict[str, Any] | None:
    headers: dict[str, str] = {}
    while True:
        line = stream.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        key, separator, value = line.partition(b":")
        if separator:
            headers[key.decode("ascii", errors="replace").strip().lower()] = value.decode(
                "ascii", errors="replace"
            ).strip()
    if "content-length" not in headers:
        raise RuntimeError(f"LSP response has no Content-Length header: {headers}")
    length = int(headers["content-length"])
    payload_parts = bytearray()
    while len(payload_parts) < length:
        chunk = stream.read(length - len(payload_parts))
        if not chunk:
            break
        payload_parts.extend(chunk)
    payload = bytes(payload_parts)
    if len(payload) != length:
        raise RuntimeError(f"Truncated LSP response: expected {length}, got {len(payload)} bytes")
    try:
        return json.loads(payload.decode("utf-8"))
    except json.JSONDecodeError as error:
        start = max(0, error.pos - 100)
        end = min(len(payload), error.pos + 100)
        raise RuntimeError(
            f"Invalid LSP JSON near byte {error.pos}: {payload[start:end]!r}"
        ) from error


def send_message(stream: BinaryIO, message: dict[str, Any]) -> None:
    payload = json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    stream.write(f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii") + payload)
    stream.flush()


class LspClient:
    def __init__(self, executable: Path, workspace: Path) -> None:
        self.process = subprocess.Popen(
            [str(executable), "lsp"], cwd=workspace, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=0,
        )
        assert self.process.stdin and self.process.stdout and self.process.stderr
        self.incoming: queue.Queue[dict[str, Any] | None] = queue.Queue()
        self.stderr_chunks: list[bytes] = []
        threading.Thread(target=self._read_stdout, daemon=True).start()
        threading.Thread(target=self._read_stderr, daemon=True).start()

    def _read_stdout(self) -> None:
        assert self.process.stdout
        try:
            while True:
                message = read_message(self.process.stdout)
                self.incoming.put(message)
                if message is None:
                    return
        except Exception as error:
            self.incoming.put({"__reader_error__": str(error)})

    def _read_stderr(self) -> None:
        assert self.process.stderr
        while True:
            chunk = self.process.stderr.read(4096)
            if not chunk:
                return
            self.stderr_chunks.append(chunk)

    def send(self, message: dict[str, Any]) -> None:
        if not self.process.stdin:
            raise RuntimeError("LSP stdin is unavailable")
        send_message(self.process.stdin, message)

    def receive(self, timeout: float = 10.0) -> dict[str, Any]:
        try:
            message = self.incoming.get(timeout=timeout)
        except queue.Empty as error:
            stderr = b"".join(self.stderr_chunks).decode("utf-8", errors="replace")
            raise RuntimeError(f"Timed out waiting for LSP message. stderr: {stderr}") from error
        if message is None:
            stderr = b"".join(self.stderr_chunks).decode("utf-8", errors="replace")
            raise RuntimeError(f"LSP closed stdout unexpectedly. stderr: {stderr}")
        if "__reader_error__" in message:
            raise RuntimeError(f"Could not parse LSP output: {message['__reader_error__']}")
        return message

    def response(self, request_id: int, timeout: float = 10.0) -> dict[str, Any]:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            message = self.receive(max(0.05, deadline - time.monotonic()))
            if message.get("id") == request_id and ("result" in message or "error" in message):
                if "error" in message:
                    raise RuntimeError(f"LSP request {request_id} failed: {message['error']}")
                return message
        raise RuntimeError(f"Timed out waiting for response to request {request_id}")

    def close(self) -> None:
        if self.process.poll() is not None:
            return
        try:
            self.send({"jsonrpc": "2.0", "id": 99, "method": "shutdown", "params": {}})
            self.response(99, timeout=3.0)
            self.send({"jsonrpc": "2.0", "method": "exit", "params": {}})
            if self.process.stdin:
                self.process.stdin.close()
            self.process.wait(timeout=3.0)
        except Exception:
            self.process.terminate()
            try:
                self.process.wait(timeout=3.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=3.0)


def assert_definition(result: Any, expected_uri: str, context: str) -> None:
    # LSP permits either a Location object or a list of Location objects.
    locations = result if isinstance(result, list) else [result] if isinstance(result, dict) else []
    if not any(
        isinstance(location, dict) and location.get("uri") == expected_uri for location in locations
    ):
        raise AssertionError(f"{context}: expected definition in {expected_uri}, got {result!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nexus", required=True, type=Path)
    executable = parser.parse_args().nexus.resolve()
    if not executable.is_file():
        raise FileNotFoundError(f"NEXUS executable not found: {executable}")

    with tempfile.TemporaryDirectory(prefix="nexus-lsp-watch-") as temporary:
        workspace = Path(temporary)
        (workspace / "nexus.toml").write_text(
            '[package]\nname = "lsp-watch-smoke"\nversion = "0.1.0"\n', encoding="utf-8"
        )
        caller = workspace / "caller.nx"
        old_helper = workspace / "disk_helper.nx"
        new_helper = workspace / "renamed_helper.nx"
        caller_text = "fn caller() { disk_helper() }\n"
        caller.write_text(caller_text, encoding="utf-8")
        old_helper.write_text("fn disk_helper() {}\n", encoding="utf-8")

        caller_uri, old_uri, new_uri = caller.as_uri(), old_helper.as_uri(), new_helper.as_uri()
        client = LspClient(executable, workspace)
        try:
            client.send({
                "jsonrpc": "2.0", "id": 1, "method": "initialize",
                "params": {
                    "rootUri": workspace.as_uri(),
                    "capabilities": {
                        "workspace": {"didChangeWatchedFiles": {"dynamicRegistration": True}}
                    },
                },
            })
            client.response(1)
            client.send({"jsonrpc": "2.0", "method": "initialized", "params": {}})
            registration = client.receive()
            if registration.get("method") != "client/registerCapability":
                raise AssertionError(f"Expected dynamic file-watcher registration, got {registration!r}")
            client.send({"jsonrpc": "2.0", "id": registration.get("id"), "result": None})

            client.send({
                "jsonrpc": "2.0", "method": "textDocument/didOpen",
                "params": {"textDocument": {
                    "uri": caller_uri, "languageId": "nx", "version": 1, "text": caller_text
                }},
            })

            def definition(request_id: int, source_text: str, expected_uri: str | None) -> None:
                position = source_text.index("disk_helper" if "disk_helper" in source_text else "renamed_helper") + 2
                client.send({
                    "jsonrpc": "2.0", "id": request_id, "method": "textDocument/definition",
                    "params": {
                        "textDocument": {"uri": caller_uri},
                        "position": {"line": 0, "character": position},
                    },
                })
                result = client.response(request_id)["result"]
                if expected_uri is None:
                    if result not in ([], None):
                        raise AssertionError(f"Deleted helper still resolves: {result!r}")
                else:
                    assert_definition(result, expected_uri, f"Definition lookup {request_id} failed")

            definition(2, caller_text, old_uri)

            # Change a cached file's contents; the running server must refresh its index.
            old_helper.write_text("fn renamed_helper() {}\n", encoding="utf-8")
            caller_text = "fn caller() { renamed_helper() }\n"
            client.send({
                "jsonrpc": "2.0", "method": "textDocument/didChange",
                "params": {
                    "textDocument": {"uri": caller_uri, "version": 2},
                    "contentChanges": [{"text": caller_text}],
                },
            })
            client.send({
                "jsonrpc": "2.0", "method": "workspace/didChangeWatchedFiles",
                "params": {"changes": [{"uri": old_uri, "type": 2}]},
            })
            definition(3, caller_text, old_uri)

            # Simulate a filesystem rename event (delete old path, create new path).
            new_helper.write_text("fn renamed_helper() {}\n", encoding="utf-8")
            old_helper.unlink()
            client.send({
                "jsonrpc": "2.0", "method": "workspace/didChangeWatchedFiles",
                "params": {"changes": [
                    {"uri": old_uri, "type": 3}, {"uri": new_uri, "type": 1}
                ]},
            })
            definition(4, caller_text, new_uri)

            new_helper.unlink()
            client.send({
                "jsonrpc": "2.0", "method": "workspace/didChangeWatchedFiles",
                "params": {"changes": [{"uri": new_uri, "type": 3}]},
            })
            definition(5, caller_text, None)
            print("LSP watched-file smoke: PASS (edit, rename, delete)")
        finally:
            client.close()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"LSP watched-file smoke: FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
