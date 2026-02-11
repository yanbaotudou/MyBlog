#include "db/Database.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>

namespace blog {
namespace {

bool isApplied(sqlite3* db, const std::string& filename, std::string& error) {
  const char* sql = "SELECT COUNT(1) FROM schema_migrations WHERE filename = ?;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    error = sqlite3_errmsg(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
  const int rc = sqlite3_step(stmt);
  bool applied = false;
  if (rc == SQLITE_ROW) {
    applied = sqlite3_column_int(stmt, 0) > 0;
  } else {
    error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return applied;
}

bool markApplied(sqlite3* db, const std::string& filename, std::string& error) {
  const char* sql = "INSERT INTO schema_migrations(filename) VALUES (?);";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    error = sqlite3_errmsg(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
  const int rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

std::string readFile(const std::filesystem::path& path, std::string& error) {
  std::ifstream in(path);
  if (!in.is_open()) {
    error = "failed to open migration file: " + path.string();
    return "";
  }
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return content;
}

}  // namespace

bool runMigrations(const Database& db,
                   const std::string& migrationsDir,
                   std::vector<std::string>& applied,
                   std::string& error) {
  if (!std::filesystem::exists(migrationsDir)) {
    error = "migrations directory does not exist: " + migrationsDir;
    return false;
  }

  std::string dbError;
  sqlite3* conn = db.open(dbError);
  if (conn == nullptr) {
    error = dbError;
    return false;
  }

  const std::string initSql =
      "CREATE TABLE IF NOT EXISTS schema_migrations ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "filename TEXT NOT NULL UNIQUE,"
      "applied_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
      ");";

  if (!db.exec(conn, initSql, error)) {
    sqlite3_close(conn);
    return false;
  }

  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(migrationsDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".sql") {
      files.push_back(entry.path());
    }
  }

  std::sort(files.begin(), files.end());

  for (const auto& file : files) {
    const std::string filename = file.filename().string();

    const bool alreadyApplied = isApplied(conn, filename, error);
    if (!error.empty()) {
      sqlite3_close(conn);
      return false;
    }
    if (alreadyApplied) {
      continue;
    }

    std::string readError;
    const std::string sql = readFile(file, readError);
    if (!readError.empty()) {
      error = readError;
      sqlite3_close(conn);
      return false;
    }

    if (!db.exec(conn, "BEGIN;", error)) {
      sqlite3_close(conn);
      return false;
    }

    if (!db.exec(conn, sql, error)) {
      std::string rollbackError;
      db.exec(conn, "ROLLBACK;", rollbackError);
      sqlite3_close(conn);
      return false;
    }

    if (!markApplied(conn, filename, error)) {
      std::string rollbackError;
      db.exec(conn, "ROLLBACK;", rollbackError);
      sqlite3_close(conn);
      return false;
    }

    if (!db.exec(conn, "COMMIT;", error)) {
      sqlite3_close(conn);
      return false;
    }

    applied.push_back(filename);
  }

  sqlite3_close(conn);
  return true;
}

}  // namespace blog
