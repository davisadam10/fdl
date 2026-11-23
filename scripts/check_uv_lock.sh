#!/usr/bin/env bash
set -euo pipefail
# Check that uv.lock contains only public registry URLs.
# Used as a pre-commit hook to prevent private/corporate indexes leaking in.
LOCKFILE="${1:-uv.lock}"
BAD=$(grep -E "https?://" "$LOCKFILE" | grep -Ev 'https?://([A-Za-z0-9-]+\.)?(pypi\.org|files\.pythonhosted\.org|github\.com)(/|"|$)' | head -n 5 || true)
if [ -n "$BAD" ]; then
    echo "ERROR: $LOCKFILE contains non-public URLs:"
    echo "$BAD"
    echo "Run: python scripts/uv_lock.py"
    exit 1
fi
