#include "auth/RefreshTokenService.h"

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <cctype>

namespace blog {
namespace {

constexpr char kHex[] = "0123456789abcdef";

std::string trim(const std::string& input) {
  size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
    start++;
  }
  size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    end--;
  }
  return input.substr(start, end - start);
}

}  // namespace

RefreshTokenService::RefreshTokenService(const Database& db, const AppConfig& config)
    : db_(db), cookieSecure_(config.refreshCookieSecure()), refreshExpireDays_(config.refreshExpireDays) {}

std::string RefreshTokenService::generateToken() const {
  unsigned char bytes[32];
  if (RAND_bytes(bytes, static_cast<int>(sizeof(bytes))) != 1) {
    return "";
  }

  std::string token;
  token.reserve(sizeof(bytes) * 2);
  for (const unsigned char b : bytes) {
    token.push_back(kHex[(b >> 4) & 0x0F]);
    token.push_back(kHex[b & 0x0F]);
  }
  return token;
}

std::string RefreshTokenService::sha256Hex(const std::string& input) const {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest);

  std::string out;
  out.reserve(SHA256_DIGEST_LENGTH * 2);
  for (unsigned char c : digest) {
    out.push_back(kHex[(c >> 4) & 0x0F]);
    out.push_back(kHex[c & 0x0F]);
  }
  return out;
}

bool RefreshTokenService::issueToken(int64_t userId, std::string& rawToken, std::string& error) {
  rawToken = generateToken();
  if (rawToken.empty()) {
    error = "failed to generate refresh token";
    return false;
  }

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    error = dbError;
    return false;
  }

  const char* sql =
      "INSERT INTO refresh_tokens(user_id, token_hash, expires_at, created_at) "
      "VALUES(?, ?, datetime('now', ?), datetime('now'));";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    error = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  const std::string modifier = "+" + std::to_string(refreshExpireDays_) + " days";
  const std::string tokenHash = sha256Hex(rawToken);

  sqlite3_bind_int64(stmt, 1, userId);
  sqlite3_bind_text(stmt, 2, tokenHash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, modifier.c_str(), -1, SQLITE_TRANSIENT);

  const int rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool RefreshTokenService::rotateToken(const std::string& oldRawToken,
                                      int64_t& userId,
                                      std::string& newRawToken,
                                      std::string& errorCode,
                                      std::string& errorMessage) {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorCode = "DB_ERROR";
    errorMessage = dbError;
    return false;
  }

  const std::string oldHash = sha256Hex(oldRawToken);

  sqlite3_stmt* selectStmt = nullptr;
  const char* selectSql =
      "SELECT id, user_id FROM refresh_tokens "
      "WHERE token_hash = ? AND revoked_at IS NULL AND expires_at > datetime('now') "
      "LIMIT 1;";

  if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(selectStmt, 1, oldHash.c_str(), -1, SQLITE_TRANSIENT);
  const int rcSelect = sqlite3_step(selectStmt);
  if (rcSelect != SQLITE_ROW) {
    sqlite3_finalize(selectStmt);
    sqlite3_close(db);
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "refresh token invalid or expired";
    return false;
  }

  const int64_t tokenId = sqlite3_column_int64(selectStmt, 0);
  userId = sqlite3_column_int64(selectStmt, 1);
  sqlite3_finalize(selectStmt);

  std::string sqlError;
  if (!db_.exec(db, "BEGIN;", sqlError)) {
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlError;
    return false;
  }

  sqlite3_stmt* revokeStmt = nullptr;
  const char* revokeSql = "UPDATE refresh_tokens SET revoked_at = datetime('now') WHERE id = ?;";
  if (sqlite3_prepare_v2(db, revokeSql, -1, &revokeStmt, nullptr) != SQLITE_OK) {
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    return false;
  }
  sqlite3_bind_int64(revokeStmt, 1, tokenId);
  if (sqlite3_step(revokeStmt) != SQLITE_DONE) {
    sqlite3_finalize(revokeStmt);
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    return false;
  }
  sqlite3_finalize(revokeStmt);

  newRawToken = generateToken();
  if (newRawToken.empty()) {
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "INTERNAL_ERROR";
    errorMessage = "failed to generate refresh token";
    return false;
  }

  sqlite3_stmt* insertStmt = nullptr;
  const char* insertSql =
      "INSERT INTO refresh_tokens(user_id, token_hash, expires_at, created_at) "
      "VALUES(?, ?, datetime('now', ?), datetime('now'));";

  if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    return false;
  }

  const std::string modifier = "+" + std::to_string(refreshExpireDays_) + " days";
  const std::string newHash = sha256Hex(newRawToken);

  sqlite3_bind_int64(insertStmt, 1, userId);
  sqlite3_bind_text(insertStmt, 2, newHash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 3, modifier.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    sqlite3_finalize(insertStmt);
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    return false;
  }
  sqlite3_finalize(insertStmt);

  if (!db_.exec(db, "COMMIT;", sqlError)) {
    db_.exec(db, "ROLLBACK;", sqlError);
    sqlite3_close(db);
    errorCode = "DB_ERROR";
    errorMessage = sqlError;
    return false;
  }

  sqlite3_close(db);
  return true;
}

bool RefreshTokenService::revokeToken(const std::string& rawToken, std::string& error) {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    error = dbError;
    return false;
  }

  const char* sql =
      "UPDATE refresh_tokens SET revoked_at = datetime('now') "
      "WHERE token_hash = ? AND revoked_at IS NULL;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    error = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  const std::string hash = sha256Hex(rawToken);
  sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool RefreshTokenService::revokeAllByUserId(int64_t userId, std::string& error) {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    error = dbError;
    return false;
  }

  const char* sql =
      "UPDATE refresh_tokens SET revoked_at = datetime('now') "
      "WHERE user_id = ? AND revoked_at IS NULL;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    error = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, userId);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    error = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

std::string RefreshTokenService::buildSetCookie(const std::string& rawToken, bool clear) const {
  std::ostringstream out;
  out << "refresh_token=";
  if (!clear) {
    out << rawToken;
  }
  out << "; Path=/api/auth; HttpOnly; SameSite=Lax";

  if (cookieSecure_) {
    out << "; Secure";
  }

  if (clear) {
    out << "; Max-Age=0";
  } else {
    out << "; Max-Age=" << (refreshExpireDays_ * 24 * 60 * 60);
  }

  return out.str();
}

std::optional<std::string> RefreshTokenService::extractRefreshToken(const drogon::HttpRequestPtr& req) {
  const auto& cookieToken = req->getCookie("refresh_token");
  if (!cookieToken.empty()) {
    return cookieToken;
  }

  const std::string cookieHeader = req->getHeader("Cookie");
  if (cookieHeader.empty()) {
    return std::nullopt;
  }

  const std::string needle = "refresh_token=";
  size_t pos = cookieHeader.find(needle);
  if (pos == std::string::npos) {
    return std::nullopt;
  }

  pos += needle.size();
  size_t end = cookieHeader.find(';', pos);
  if (end == std::string::npos) {
    end = cookieHeader.size();
  }

  const std::string token = trim(cookieHeader.substr(pos, end - pos));
  if (token.empty()) {
    return std::nullopt;
  }
  return token;
}

}  // namespace blog
