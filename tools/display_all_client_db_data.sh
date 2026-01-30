#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [[ -z "${repo_root}" ]]; then
  echo "Could not determine git repo root." >&2
  exit 1
fi

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "sqlite3 is required but was not found in PATH." >&2
  exit 1
fi

shopt -s nullglob
db_files=("${repo_root}"/client_*.db)
shopt -u nullglob

if [[ ${#db_files[@]} -eq 0 ]]; then
  echo "No client databases found (client_*.db) in ${repo_root}."
  exit 0
fi

for db in "${db_files[@]}"; do
  echo "=== $(basename "${db}") ==="
  sqlite3 -header -column "${db}" "SELECT * FROM banned_users;" 2>/dev/null \
    || echo "  (table 'banned_users' not found or empty)"
  echo
done
