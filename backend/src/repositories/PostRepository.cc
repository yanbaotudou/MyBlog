#include "repositories/PostRepository.h"

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

Post rowToPost(sqlite3_stmt* stmt) {
  Post post;
  post.id = sqlite3_column_int64(stmt, 0);
  post.title = textOrEmpty(stmt, 1);
  post.contentMarkdown = textOrEmpty(stmt, 2);
  post.authorId = sqlite3_column_int64(stmt, 3);
  post.authorUsername = textOrEmpty(stmt, 4);
  post.createdAt = textOrEmpty(stmt, 5);
  post.updatedAt = textOrEmpty(stmt, 6);
  post.isDeleted = sqlite3_column_int(stmt, 7) != 0;
  return post;
}

}  // namespace

PostRepository::PostRepository(const Database& db) : db_(db) {}

bool PostRepository::listPosts(int page,
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

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT COUNT(1) FROM posts WHERE is_deleted = 0;",
                         -1,
                         &countStmt,
                         nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM posts p "
      "JOIN users u ON u.id = p.author_id "
      "WHERE p.is_deleted = 0 "
      "ORDER BY p.updated_at DESC "
      "LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int(stmt, 1, pageSize);
  sqlite3_bind_int(stmt, 2, (page - 1) * pageSize);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    posts.push_back(rowToPost(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool PostRepository::listPostsByAuthor(int64_t authorId,
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

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT COUNT(1) FROM posts WHERE author_id = ? AND is_deleted = 0;",
                         -1,
                         &countStmt,
                         nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(countStmt, 1, authorId);

  total = 0;
  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM posts p "
      "JOIN users u ON u.id = p.author_id "
      "WHERE p.author_id = ? AND p.is_deleted = 0 "
      "ORDER BY p.updated_at DESC "
      "LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, authorId);
  sqlite3_bind_int(stmt, 2, pageSize);
  sqlite3_bind_int(stmt, 3, (page - 1) * pageSize);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    posts.push_back(rowToPost(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

std::optional<Post> PostRepository::findById(int64_t id, bool includeDeleted) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    return std::nullopt;
  }

  const char* sql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM posts p "
      "JOIN users u ON u.id = p.author_id "
      "WHERE p.id = ? AND (? = 1 OR p.is_deleted = 0) "
      "LIMIT 1;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return std::nullopt;
  }

  sqlite3_bind_int64(stmt, 1, id);
  sqlite3_bind_int(stmt, 2, includeDeleted ? 1 : 0);

  std::optional<Post> post;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    post = rowToPost(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return post;
}

bool PostRepository::createPost(const std::string& title,
                                const std::string& contentMarkdown,
                                int64_t authorId,
                                Post& out,
                                std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* insertSql =
      "INSERT INTO posts(title, content_markdown, author_id, created_at, updated_at, is_deleted) "
      "VALUES(?, ?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'), strftime('%Y-%m-%dT%H:%M:%SZ','now'), 0);";

  sqlite3_stmt* insertStmt = nullptr;
  if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(insertStmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 2, contentMarkdown.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(insertStmt, 3, authorId);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(insertStmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(insertStmt);
  const int64_t newId = sqlite3_last_insert_rowid(db);

  const char* querySql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted "
      "FROM posts p JOIN users u ON u.id = p.author_id WHERE p.id = ? LIMIT 1;";

  sqlite3_stmt* queryStmt = nullptr;
  if (sqlite3_prepare_v2(db, querySql, -1, &queryStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(queryStmt, 1, newId);
  if (sqlite3_step(queryStmt) == SQLITE_ROW) {
    out = rowToPost(queryStmt);
  }

  sqlite3_finalize(queryStmt);
  sqlite3_close(db);
  return true;
}

bool PostRepository::updatePost(int64_t id,
                                const std::string& title,
                                const std::string& contentMarkdown,
                                std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "UPDATE posts SET title = ?, content_markdown = ?, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') "
      "WHERE id = ? AND is_deleted = 0;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, contentMarkdown.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, id);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  const int changed = sqlite3_changes(db);
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  if (changed == 0) {
    errorMessage = "post not found";
    return false;
  }

  return true;
}

bool PostRepository::softDeletePost(int64_t id, std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "UPDATE posts SET is_deleted = 1, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') "
      "WHERE id = ? AND is_deleted = 0;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, id);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  const int changed = sqlite3_changes(db);
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  if (changed == 0) {
    errorMessage = "post not found";
    return false;
  }

  return true;
}

}  // namespace blog
