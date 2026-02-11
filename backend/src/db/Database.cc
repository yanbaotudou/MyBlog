#include "db/Database.h"

#include <filesystem>

#include <drogon/drogon.h>

namespace blog {

Database::Database(std::string dbPath) : dbPath_(std::move(dbPath)) {}

const std::string& Database::path() const {
  return dbPath_;
}

bool Database::ensureParentDir(std::string& error) const {
  try {
    const std::filesystem::path p(dbPath_);
    const auto parent = p.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
      std::filesystem::create_directories(parent);
    }
    return true;
  } catch (const std::exception& ex) {
    error = ex.what();
    return false;
  }
}

sqlite3* Database::open(std::string& error) const {
  sqlite3* db = nullptr;
  const int rc = sqlite3_open_v2(
      dbPath_.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);

  if (rc != SQLITE_OK) {
    error = db ? sqlite3_errmsg(db) : "failed to open database";
    if (db != nullptr) {
      sqlite3_close(db);
    }
    return nullptr;
  }

  std::string pragmaError;
  if (!exec(db, "PRAGMA foreign_keys = ON;", pragmaError)) {
    LOG_ERROR << "Failed to set PRAGMA foreign_keys: " << pragmaError;
  }
  if (!exec(db, "PRAGMA journal_mode = WAL;", pragmaError)) {
    LOG_ERROR << "Failed to set PRAGMA journal_mode: " << pragmaError;
  }

  return db;
}

bool Database::exec(sqlite3* db, const std::string& sql, std::string& error) const {
  char* errMsg = nullptr;
  const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    if (errMsg != nullptr) {
      error = errMsg;
      sqlite3_free(errMsg);
    } else {
      error = "sqlite exec failed";
    }
    return false;
  }
  return true;
}

}  // namespace blog
