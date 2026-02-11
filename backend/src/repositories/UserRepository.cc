#include "repositories/UserRepository.h"

#include <sqlite3.h>

namespace blog {
namespace {

std::string textOrEmpty(sqlite3_stmt* stmt, int col) {
  const auto* text = sqlite3_column_text(stmt, col);
  if (text == nullptr) {
    return "";
  }
  return reinterpret_cast<const char*>(text);
}

User rowToUser(sqlite3_stmt* stmt) {
  User user;
  user.id = sqlite3_column_int64(stmt, 0);
  user.username = textOrEmpty(stmt, 1);
  user.passwordHash = textOrEmpty(stmt, 2);
  user.role = textOrEmpty(stmt, 3);
  user.isBanned = sqlite3_column_int(stmt, 4) != 0;
  user.createdAt = textOrEmpty(stmt, 5);
  return user;
}

}  // namespace

UserRepository::UserRepository(const Database& db) : db_(db) {}

std::optional<User> UserRepository::findByUsername(const std::string& username) const {
  std::string error;
  sqlite3* db = db_.open(error);
  if (db == nullptr) {
    return std::nullopt;
  }

  const char* sql =
      "SELECT id, username, password_hash, role, is_banned, created_at "
      "FROM users WHERE username = ? LIMIT 1;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return std::nullopt;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

  std::optional<User> user;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    user = rowToUser(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return user;
}

std::optional<User> UserRepository::findById(int64_t id) const {
  std::string error;
  sqlite3* db = db_.open(error);
  if (db == nullptr) {
    return std::nullopt;
  }

  const char* sql =
      "SELECT id, username, password_hash, role, is_banned, created_at "
      "FROM users WHERE id = ? LIMIT 1;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return std::nullopt;
  }

  sqlite3_bind_int64(stmt, 1, id);

  std::optional<User> user;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    user = rowToUser(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return user;
}

bool UserRepository::createUser(const std::string& username,
                                const std::string& passwordHash,
                                const std::string& role,
                                User& out,
                                std::string& errorCode,
                                std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorCode = "DB_ERROR";
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "INSERT INTO users(username, password_hash, role, is_banned, created_at) "
      "VALUES(?, ?, ?, 0, strftime('%Y-%m-%dT%H:%M:%SZ','now'));";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);

  const int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    if (rc == SQLITE_CONSTRAINT || rc == SQLITE_CONSTRAINT_UNIQUE) {
      errorCode = "USERNAME_EXISTS";
      errorMessage = "username already exists";
    } else {
      errorCode = "DB_ERROR";
      errorMessage = sqlite3_errmsg(db);
    }
    sqlite3_close(db);
    return false;
  }

  const int64_t newId = sqlite3_last_insert_rowid(db);

  const char* query =
      "SELECT id, username, password_hash, role, is_banned, created_at "
      "FROM users WHERE id = ? LIMIT 1;";

  sqlite3_stmt* qStmt = nullptr;
  if (sqlite3_prepare_v2(db, query, -1, &qStmt, nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(qStmt, 1, newId);
  if (sqlite3_step(qStmt) == SQLITE_ROW) {
    out = rowToUser(qStmt);
  }
  sqlite3_finalize(qStmt);
  sqlite3_close(db);

  return true;
}

bool UserRepository::ensureDefaultAdmin(const std::string& username,
                                        const std::string& passwordHash,
                                        bool& created,
                                        std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* existsSql = "SELECT COUNT(1) FROM users WHERE username = ? LIMIT 1;";
  sqlite3_stmt* existsStmt = nullptr;
  if (sqlite3_prepare_v2(db, existsSql, -1, &existsStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(existsStmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(existsStmt) == SQLITE_ROW) {
    if (sqlite3_column_int(existsStmt, 0) > 0) {
      created = false;
      sqlite3_finalize(existsStmt);
      sqlite3_close(db);
      return true;
    }
  }
  sqlite3_finalize(existsStmt);

  const char* insertSql =
      "INSERT INTO users(username, password_hash, role, is_banned, created_at) "
      "VALUES(?, ?, 'admin', 0, strftime('%Y-%m-%dT%H:%M:%SZ','now'));";

  sqlite3_stmt* insertStmt = nullptr;
  if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(insertStmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(insertStmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(insertStmt);
  sqlite3_close(db);
  created = true;
  return true;
}

bool UserRepository::listUsers(int page,
                               int pageSize,
                               std::vector<User>& users,
                               int& total,
                               std::string& errorMessage) const {
  users.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT COUNT(1) FROM users;", -1, &countStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "SELECT id, username, password_hash, role, is_banned, created_at "
      "FROM users ORDER BY id ASC LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int(stmt, 1, pageSize);
  sqlite3_bind_int(stmt, 2, (page - 1) * pageSize);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    users.push_back(rowToUser(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool UserRepository::updateRole(int64_t userId, const std::string& role, std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql = "UPDATE users SET role = ? WHERE id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, userId);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  const int changed = sqlite3_changes(db);
  sqlite3_finalize(stmt);

  if (changed == 0) {
    sqlite3_stmt* existsStmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(1) FROM users WHERE id = ?;", -1, &existsStmt, nullptr) !=
        SQLITE_OK) {
      errorMessage = sqlite3_errmsg(db);
      sqlite3_close(db);
      return false;
    }
    sqlite3_bind_int64(existsStmt, 1, userId);
    const int rc = sqlite3_step(existsStmt);
    const bool exists = (rc == SQLITE_ROW && sqlite3_column_int(existsStmt, 0) > 0);
    sqlite3_finalize(existsStmt);
    sqlite3_close(db);
    if (exists) {
      return true;
    }
    errorMessage = "user not found";
    return false;
  }

  sqlite3_close(db);
  return true;
}

bool UserRepository::updateBanStatus(int64_t userId, bool isBanned, std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql = "UPDATE users SET is_banned = ? WHERE id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int(stmt, 1, isBanned ? 1 : 0);
  sqlite3_bind_int64(stmt, 2, userId);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  const int changed = sqlite3_changes(db);
  sqlite3_finalize(stmt);

  if (changed == 0) {
    sqlite3_stmt* existsStmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(1) FROM users WHERE id = ?;", -1, &existsStmt, nullptr) !=
        SQLITE_OK) {
      errorMessage = sqlite3_errmsg(db);
      sqlite3_close(db);
      return false;
    }
    sqlite3_bind_int64(existsStmt, 1, userId);
    const int rc = sqlite3_step(existsStmt);
    const bool exists = (rc == SQLITE_ROW && sqlite3_column_int(existsStmt, 0) > 0);
    sqlite3_finalize(existsStmt);
    sqlite3_close(db);
    if (exists) {
      return true;
    }
    errorMessage = "user not found";
    return false;
  }

  sqlite3_close(db);
  return true;
}

bool UserRepository::updatePasswordHash(int64_t userId,
                                        const std::string& passwordHash,
                                        std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql = "UPDATE users SET password_hash = ? WHERE id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, userId);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  const int changed = sqlite3_changes(db);
  sqlite3_finalize(stmt);

  if (changed == 0) {
    sqlite3_stmt* existsStmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(1) FROM users WHERE id = ?;", -1, &existsStmt, nullptr) !=
        SQLITE_OK) {
      errorMessage = sqlite3_errmsg(db);
      sqlite3_close(db);
      return false;
    }
    sqlite3_bind_int64(existsStmt, 1, userId);
    const int rc = sqlite3_step(existsStmt);
    const bool exists = (rc == SQLITE_ROW && sqlite3_column_int(existsStmt, 0) > 0);
    sqlite3_finalize(existsStmt);
    sqlite3_close(db);
    if (exists) {
      return true;
    }
    errorMessage = "user not found";
    return false;
  }

  sqlite3_close(db);
  return true;
}

}  // namespace blog
