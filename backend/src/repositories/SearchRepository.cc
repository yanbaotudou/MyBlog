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

std::string escapeLikePattern(const std::string& input) {
  std::string escaped;
  escaped.reserve(input.size() * 2);
  for (const char c : input) {
    if (c == '\\' || c == '%' || c == '_') {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
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
  const std::string likePattern = "%" + escapeLikePattern(q) + "%";

  const char* countSql =
      "WITH fts_hits AS ("
      "  SELECT p.id AS id "
      "  FROM posts_fts "
      "  JOIN posts p ON p.id = posts_fts.rowid "
      "  WHERE posts_fts MATCH ? AND p.is_deleted = 0"
      "), like_hits AS ("
      "  SELECT p.id AS id "
      "  FROM posts p "
      "  WHERE p.is_deleted = 0 "
      "    AND (p.title LIKE ? ESCAPE '\\' OR p.content_markdown LIKE ? ESCAPE '\\')"
      "), merged AS ("
      "  SELECT id FROM fts_hits "
      "  UNION "
      "  SELECT id FROM like_hits"
      ") "
      "SELECT COUNT(1) FROM merged;";

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db, countSql, -1, &countStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(countStmt, 1, ftsQuery.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(countStmt, 2, likePattern.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(countStmt, 3, likePattern.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "WITH fts_hits AS ("
      "  SELECT p.id AS id, bm25(posts_fts) AS score "
      "  FROM posts_fts "
      "  JOIN posts p ON p.id = posts_fts.rowid "
      "  WHERE posts_fts MATCH ? AND p.is_deleted = 0"
      "), like_hits AS ("
      "  SELECT p.id AS id, 1000.0 AS score "
      "  FROM posts p "
      "  WHERE p.is_deleted = 0 "
      "    AND (p.title LIKE ? ESCAPE '\\' OR p.content_markdown LIKE ? ESCAPE '\\')"
      "), merged AS ("
      "  SELECT id, MIN(score) AS score FROM ("
      "    SELECT id, score FROM fts_hits "
      "    UNION ALL "
      "    SELECT id, score FROM like_hits"
      "  ) GROUP BY id"
      ") "
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM merged m "
      "JOIN posts p ON p.id = m.id "
      "JOIN users u ON u.id = p.author_id "
      "ORDER BY m.score ASC, p.updated_at DESC "
      "LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, ftsQuery.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, likePattern.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, likePattern.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 4, pageSize);
  sqlite3_bind_int(stmt, 5, (page - 1) * pageSize);

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
