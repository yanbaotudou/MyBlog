#include "repositories/SearchRepository.h"

#include <sqlite3.h>

#include <sstream>
#include <cctype>

namespace blog {
namespace {

std::string textOrEmpty(sqlite3_stmt* stmt, int col) {
  const auto* text = sqlite3_column_text(stmt, col);
  if (text == nullptr) {
    return "";
  }
  return reinterpret_cast<const char*>(text);
}

bool isAsciiAlnumToken(const std::string& token) {
  if (token.empty()) {
    return false;
  }
  for (const unsigned char c : token) {
    if (!(std::isalnum(c) || c == '_')) {
      return false;
    }
  }
  return true;
}

}  // namespace

SearchRepository::SearchRepository(const Database& db) : db_(db) {}

std::string SearchRepository::toFtsQuery(const std::string& q) const {
  std::stringstream ss(q);
  std::string token;
  std::vector<std::string> parts;
  while (ss >> token) {
    if (token.empty()) {
      continue;
    }

    std::string safe = token;
    for (char& c : safe) {
      if (c == '"') {
        c = ' ';
      }
    }

    if (isAsciiAlnumToken(safe)) {
      parts.push_back(safe + "*");
    } else {
      parts.push_back("\"" + safe + "\"");
    }
  }

  if (parts.empty()) {
    std::string safe = q;
    for (char& c : safe) {
      if (c == '"') {
        c = ' ';
      }
    }
    return "\"" + safe + "\"";
  }

  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out += " OR ";
    }
    out += parts[i];
  }
  return out;
}

bool SearchRepository::searchPosts(const std::string& q,
                                   int page,
                                   int pageSize,
                                   std::vector<Post>& posts,
                                   int& total,
                                   std::string& errorMessage) const {
  posts.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const std::string ftsQuery = toFtsQuery(q);

  const char* countSql =
      "SELECT COUNT(1) "
      "FROM posts_fts "
      "JOIN posts p ON p.id = posts_fts.rowid "
      "WHERE posts_fts MATCH ? AND p.is_deleted = 0;";

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db, countSql, -1, &countStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(countStmt, 1, ftsQuery.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM posts_fts "
      "JOIN posts p ON p.id = posts_fts.rowid "
      "JOIN users u ON u.id = p.author_id "
      "WHERE posts_fts MATCH ? AND p.is_deleted = 0 "
      "ORDER BY bm25(posts_fts), p.updated_at DESC "
      "LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, ftsQuery.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, pageSize);
  sqlite3_bind_int(stmt, 3, (page - 1) * pageSize);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    Post post;
    post.id = sqlite3_column_int64(stmt, 0);
    post.title = textOrEmpty(stmt, 1);
    post.contentMarkdown = textOrEmpty(stmt, 2);
    post.authorId = sqlite3_column_int64(stmt, 3);
    post.authorUsername = textOrEmpty(stmt, 4);
    post.createdAt = textOrEmpty(stmt, 5);
    post.updatedAt = textOrEmpty(stmt, 6);
    post.isDeleted = sqlite3_column_int(stmt, 7) != 0;
    posts.push_back(post);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

}  // namespace blog
