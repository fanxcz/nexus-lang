#!/usr/bin/env python3
"""Create/update GitHub repository labels from .github/labels.json."""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


def api_request(url: str, token: str, method: str = "GET", payload: dict | None = None):
    data = None if payload is None else json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=data,
        method=method,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2022-11-28",
            "Content-Type": "application/json",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            body = response.read()
            return json.loads(body) if body else None
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"GitHub API {method} {url} failed ({error.code}): {detail}") from error


def main() -> int:
    repository = os.environ.get("GH_REPO", os.environ.get("GITHUB_REPOSITORY", "")).strip()
    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", "")).strip()
    if not repository or "/" not in repository:
        print("error: set GH_REPO=owner/name", file=sys.stderr)
        return 2
    if not token:
        print("error: set GH_TOKEN (or GITHUB_TOKEN) with Issues write permission", file=sys.stderr)
        return 2

    manifest = Path(__file__).resolve().parents[1] / ".github" / "labels.json"
    labels = json.loads(manifest.read_text(encoding="utf-8"))
    base = f"https://api.github.com/repos/{urllib.parse.quote(repository, safe='/')}"
    existing = api_request(f"{base}/labels?per_page=100", token) or []
    by_name = {item["name"].casefold(): item for item in existing}

    for label in labels:
        name = label["name"]
        current = by_name.get(name.casefold())
        if current:
            url = f"{base}/labels/{urllib.parse.quote(current['name'], safe='')}"
            result = api_request(
                url, token, "PATCH",
                {"new_name": name, "color": label["color"], "description": label["description"]},
            )
            action = "updated"
        else:
            result = api_request(f"{base}/labels", token, "POST", label)
            action = "created"
        print(f"{action}: {result['name']}")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
