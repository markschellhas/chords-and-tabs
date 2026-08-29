#!/usr/bin/env python3
"""stdio MCP server: get_progressions / get_song for the chords-and-tabs agent API."""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

DEFAULT_PORT = int(os.environ.get("CHORDS_AGENT_PORT", "17891"))


def snapshot_dirs() -> list[Path]:
    home = Path.home()
    return [
        home / ".config" / "chords-and-tabs",
        home / "Library" / "Application Support" / "chords-and-tabs",
    ]


def live_url() -> str:
    for folder in snapshot_dirs():
        meta = folder / "agent-api.json"
        if meta.is_file():
            try:
                data = json.loads(meta.read_text())
                if "url" in data:
                    return str(data["url"]).rstrip("/")
            except json.JSONDecodeError:
                pass
    return f"http://127.0.0.1:{DEFAULT_PORT}"


def read_snapshot(name: str) -> str | None:
    for folder in snapshot_dirs():
        path = folder / name
        if path.is_file():
            return path.read_text()
    return None


def fetch(path: str, snapshot_name: str) -> str:
    url = live_url() + path
    try:
        with urllib.request.urlopen(url, timeout=1.5) as response:
            return response.read().decode("utf-8")
    except (urllib.error.URLError, TimeoutError, OSError):
        snapshot = read_snapshot(snapshot_name)
        if snapshot is not None:
            return snapshot
        raise RuntimeError(
            f"chords-and-tabs is not reachable at {url} and no snapshot "
            f"{snapshot_name} was found. Start the app, then retry."
        ) from None


TOOLS = [
    {
        "name": "get_progressions",
        "description": (
            "Return chord progressions that have been added to the current "
            "chords-and-tabs song, grouped by section."
        ),
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "get_song",
        "description": (
            "Return the full chords-and-tabs song document, including empty slots."
        ),
        "inputSchema": {"type": "object", "properties": {}},
    },
]


def handle_tool(name: str) -> str:
    if name == "get_progressions":
        return fetch("/progressions", "progressions.json")
    if name == "get_song":
        return fetch("/song", "song.json")
    raise RuntimeError(f"unknown tool: {name}")


def write_message(payload: dict) -> None:
    body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    sys.stdout.write(f"Content-Length: {len(body)}\r\n\r\n")
    sys.stdout.flush()
    sys.stdout.buffer.write(body)
    sys.stdout.buffer.flush()


def read_message() -> dict | None:
    headers: dict[str, str] = {}
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        key, _, value = line.decode("utf-8").partition(":")
        headers[key.strip().lower()] = value.strip()

    length = int(headers.get("content-length", "0"))
    if length <= 0:
        return None
    body = sys.stdin.buffer.read(length)
    return json.loads(body.decode("utf-8"))


def main() -> None:
    while True:
        message = read_message()
        if message is None:
            return

        method = message.get("method")
        msg_id = message.get("id")

        if method == "initialize":
            write_message(
                {
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "result": {
                        "protocolVersion": "2024-11-05",
                        "capabilities": {"tools": {}},
                        "serverInfo": {"name": "chords-and-tabs", "version": "0.1.0"},
                    },
                }
            )
        elif method == "notifications/initialized":
            continue
        elif method == "tools/list":
            write_message({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": TOOLS}})
        elif method == "tools/call":
            name = message.get("params", {}).get("name", "")
            try:
                text = handle_tool(name)
                write_message(
                    {
                        "jsonrpc": "2.0",
                        "id": msg_id,
                        "result": {"content": [{"type": "text", "text": text}]},
                    }
                )
            except Exception as exc:  # noqa: BLE001 — surface to the agent
                write_message(
                    {
                        "jsonrpc": "2.0",
                        "id": msg_id,
                        "result": {
                            "content": [{"type": "text", "text": str(exc)}],
                            "isError": True,
                        },
                    }
                )
        elif method == "ping":
            write_message({"jsonrpc": "2.0", "id": msg_id, "result": {}})
        elif msg_id is not None:
            write_message(
                {
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "error": {"code": -32601, "message": f"Method not found: {method}"},
                }
            )


if __name__ == "__main__":
    main()
