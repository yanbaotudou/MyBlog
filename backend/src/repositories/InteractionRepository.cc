#include "repositories/InteractionRepository.h"

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

Comment rowToComment(sqlite3_stmt* stmt) {
  Comment comment;
  comment.id = sqlite3_column_int64(stmt, 0);
  comment.postId = sqlite3_column_int64(stmt, 1);
  comment.userId = sqlite3_column_int64(stmt, 2);
  comment.username = textOrEmpty(stmt, 3);
  comment.content = textOrEmpty(stmt, 4);
  comment.createdAt = textOrEmpty(stmt, 5);
  comment.updatedAt = textOrEmpty(stmt, 6);
  comment.isDeleted = sqlite3_column_int(stmt, 7) != 0;
  return comment;
}

bool execLikeMutation(sqlite3* db,
                      const char* insertSql,
                      const char* deleteSql,
                      int64_t postId,
                      int64_t userId,
                      bool enabled,
                      std::string& errorMessage) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = enabled ? insertSql : deleteSql;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, postId);
  sqlite3_bind_int64(stmt, 2, userId);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

}  // namespace

InteractionRepository::InteractionRepository(const Database& db) : db_(db) {}

bool InteractionRepository::getPostInteractionSummary(int64_t postId,
                                                      const std::optional<int64_t>& currentUserId,
                                                      PostInteractionSummary& summary,
                                                      std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "SELECT "
      "  (SELECT COUNT(1) FROM post_likes WHERE post_id = ?), "
      "  (SELECT COUNT(1) FROM post_favorites WHERE post_id = ?), "
      "  (SELECT COUNT(1) FROM comments WHERE post_id = ? AND is_deleted = 0), "
      "  (SELECT COUNT(1) FROM post_likes WHERE post_id = ? AND user_id = ?), "
      "  (SELECT COUNT(1) FROM post_favorites WHERE post_id = ? AND user_id = ?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  const int64_t uid = currentUserId.value_or(0);
  sqlite3_bind_int64(stmt, 1, postId);
  sqlite3_bind_int64(stmt, 2, postId);
  sqlite3_bind_int64(stmt, 3, postId);
  sqlite3_bind_int64(stmt, 4, postId);
  sqlite3_bind_int64(stmt, 5, uid);
  sqlite3_bind_int64(stmt, 6, postId);
  sqlite3_bind_int64(stmt, 7, uid);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    summary.likeCount = sqlite3_column_int(stmt, 0);
    summary.favoriteCount = sqlite3_column_int(stmt, 1);
    summary.commentCount = sqlite3_column_int(stmt, 2);
    summary.likedByMe = currentUserId.has_value() && sqlite3_column_int(stmt, 3) > 0;
    summary.favoritedByMe = currentUserId.has_value() && sqlite3_column_int(stmt, 4) > 0;
  } else {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool InteractionRepository::setLike(int64_t postId,
                                    int64_t userId,
                                    bool liked,
                                    std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* insertSql =
      "INSERT OR IGNORE INTO post_likes(post_id, user_id, created_at) "
      "VALUES(?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'));";
  const char* deleteSql = "DELETE FROM post_likes WHERE post_id = ? AND user_id = ?;";

  const bool ok = execLikeMutation(db, insertSql, deleteSql, postId, userId, liked, errorMessage);
  sqlite3_close(db);
  return ok;
}

bool InteractionRepository::setFavorite(int64_t postId,
                                        int64_t userId,
                                        bool favorited,
                                        std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* insertSql =
      "INSERT OR IGNORE INTO post_favorites(post_id, user_id, created_at) "
      "VALUES(?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'));";
  const char* deleteSql = "DELETE FROM post_favorites WHERE post_id = ? AND user_id = ?;";

  const bool ok = execLikeMutation(db, insertSql, deleteSql, postId, userId, favorited, errorMessage);
  sqlite3_close(db);
  return ok;
}

bool InteractionRepository::listComments(int64_t postId,
                                         int page,
                                         int pageSize,
                                         std::vector<Comment>& comments,
                                         int& total,
                                         std::string& errorMessage) const {
  comments.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* countSql =
      "SELECT COUNT(1) "
      "FROM comments "
      "WHERE post_id = ? AND is_deleted = 0;";

  sqlite3_stmt* countStmt = nullptr;
  if (sqlite3_prepare_v2(db, countSql, -1, &countStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(countStmt, 1, postId);
  total = 0;
  if (sqlite3_step(countStmt) == SQLITE_ROW) {
    total = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);

  const char* sql =
      "SELECT c.id, c.post_id, c.user_id, u.username, c.content, c.created_at, c.updated_at, c.is_deleted "
      "FROM comments c "
      "JOIN users u ON u.id = c.user_id "
      "WHERE c.post_id = ? AND c.is_deleted = 0 "
      "ORDER BY c.created_at DESC "
      "LIMIT ? OFFSET ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, postId);
  sqlite3_bind_int(stmt, 2, pageSize);
  sqlite3_bind_int(stmt, 3, (page - 1) * pageSize);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    comments.push_back(rowToComment(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool InteractionRepository::createComment(int64_t postId,
                                          int64_t userId,
                                          const std::string& content,
                                          Comment& out,
                                          std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* insertSql =
      "INSERT INTO comments(post_id, user_id, content, created_at, updated_at, is_deleted) "
      "VALUES(?, ?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'), strftime('%Y-%m-%dT%H:%M:%SZ','now'), 0);";

  sqlite3_stmt* insertStmt = nullptr;
  if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(insertStmt, 1, postId);
  sqlite3_bind_int64(insertStmt, 2, userId);
  sqlite3_bind_text(insertStmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(insertStmt);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(insertStmt);

  const int64_t newId = sqlite3_last_insert_rowid(db);
  const char* querySql =
      "SELECT c.id, c.post_id, c.user_id, u.username, c.content, c.created_at, c.updated_at, c.is_deleted "
      "FROM comments c "
      "JOIN users u ON u.id = c.user_id "
      "WHERE c.id = ? LIMIT 1;";

  sqlite3_stmt* queryStmt = nullptr;
  if (sqlite3_prepare_v2(db, querySql, -1, &queryStmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(queryStmt, 1, newId);
  if (sqlite3_step(queryStmt) == SQLITE_ROW) {
    out = rowToComment(queryStmt);
  } else {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(queryStmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(queryStmt);
  sqlite3_close(db);
  return true;
}

std::optional<Comment> InteractionRepository::findCommentById(int64_t commentId, bool includeDeleted) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    return std::nullopt;
  }

  const char* sql =
      "SELECT c.id, c.post_id, c.user_id, u.username, c.content, c.created_at, c.updated_at, c.is_deleted "
      "FROM comments c "
      "JOIN users u ON u.id = c.user_id "
      "WHERE c.id = ? AND (? = 1 OR c.is_deleted = 0) "
      "LIMIT 1;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return std::nullopt;
  }
  sqlite3_bind_int64(stmt, 1, commentId);
  sqlite3_bind_int(stmt, 2, includeDeleted ? 1 : 0);

  std::optional<Comment> result;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = rowToComment(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return result;
}

bool InteractionRepository::softDeleteComment(int64_t commentId, std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "UPDATE comments SET is_deleted = 1, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') "
      "WHERE id = ? AND is_deleted = 0;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(stmt, 1, commentId);

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
    errorMessage = "comment not found";
    return false;
  }
  return true;
}

}  // namespace blog
