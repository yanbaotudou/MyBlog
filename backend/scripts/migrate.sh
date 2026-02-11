#!/usr/bin/env bash
set -euo pipefail

DB_PATH="${DB_PATH:-./data/blog.db}"
MIGRATIONS_DIR="${MIGRATIONS_DIR:-./backend/migrations}"

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "sqlite3 command not found. Please install sqlite3 first."
  exit 1
fi

mkdir -p "$(dirname "$DB_PATH")"

sqlite3 "$DB_PATH" "CREATE TABLE IF NOT EXISTS schema_migrations (id INTEGER PRIMARY KEY AUTOINCREMENT, filename TEXT NOT NULL UNIQUE, applied_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')));"

for file in $(find "$MIGRATIONS_DIR" -maxdepth 1 -type f -name '*.sql' | sort); do
  filename="$(basename "$file")"
  exists=$(sqlite3 "$DB_PATH" "SELECT COUNT(1) FROM schema_migrations WHERE filename='$filename';")
  if [ "$exists" -eq 0 ]; then
    echo "Applying migration: $filename"
    sqlite3 "$DB_PATH" < "$file"
    sqlite3 "$DB_PATH" "INSERT INTO schema_migrations(filename) VALUES('$filename');"
  fi
done

echo "Migrations complete."
