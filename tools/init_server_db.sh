#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [[ -z "${repo_root}" ]]; then
  echo "Could not determine git repo root." >&2
  exit 1
fi

db_path="${repo_root}/server_db.db"

if [[ -f "${db_path}" ]]; then
  echo "server_db.db already exists at ${db_path}; skipping."
  exit 0
fi

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "sqlite3 is required but was not found in PATH." >&2
  exit 1
fi

sqlite3 "${db_path}" <<'SQL'
CREATE TABLE IF NOT EXISTS Statistics (
  pseudonym TEXT PRIMARY KEY,
  nb_of_connection INTEGER NOT NULL CHECK (nb_of_connection >= 0),
  tx_messages INTEGER NOT NULL CHECK (tx_messages >= 0),
  cumulated_connection_time_sec INTEGER NOT NULL CHECK (cumulated_connection_time_sec >= 0)
);
SQL

echo "Created database at ${db_path} with Statistics table."
