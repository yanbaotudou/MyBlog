#include "repositories/CollectionRepository.h"

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

Collection rowToCollection(sqlite3_stmt* stmt) {
  Collection collection;
  collection.id = sqlite3_column_int64(stmt, 0);
  collection.name = textOrEmpty(stmt, 1);
  collection.description = textOrEmpty(stmt, 2);
  collection.ownerId = sqlite3_column_int64(stmt, 3);
  collection.ownerUsername = textOrEmpty(stmt, 4);
  collection.createdAt = textOrEmpty(stmt, 5);
  collection.updatedAt = textOrEmpty(stmt, 6);
  collection.isDeleted = sqlite3_column_int(stmt, 7) != 0;
  collection.postCount = sqlite3_column_int(stmt, 8);
  return collection;
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
  post.collectionPosition = sqlite3_column_int(stmt, 8);
  return post;
}

bool rollback(const Database& dbWrap, sqlite3* db, std::string& errorMessage) {
  std::string rollbackError;
  if (!dbWrap.exec(db, "ROLLBACK;", rollbackError)) {
    errorMessage = rollbackError;
    return false;
  }
  return true;
}

}  // namespace

CollectionRepository::CollectionRepository(const Database& db) : db_(db) {}

bool CollectionRepository::createCollection(int64_t ownerId,
                                            const std::string& name,
                                            const std::string& description,
                                            Collection& out,
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
      "INSERT INTO collections(name, description, owner_id, created_at, updated_at, is_deleted) "
      "VALUES(?, ?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'), strftime('%Y-%m-%dT%H:%M:%SZ','now'), 0);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, ownerId);

  const int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    if (rc == SQLITE_CONSTRAINT || rc == SQLITE_CONSTRAINT_UNIQUE) {
      errorCode = "COLLECTION_NAME_EXISTS";
      errorMessage = "collection name already exists for this user";
    } else {
      errorCode = "DB_ERROR";
      errorMessage = sqlite3_errmsg(db);
    }
    sqlite3_close(db);
    return false;
  }

  const int64_t newId = sqlite3_last_insert_rowid(db);
  sqlite3_close(db);

  const auto created = findById(newId, false);
  if (!created.has_value()) {
    errorCode = "DB_ERROR";
    errorMessage = "failed to query created collection";
    return false;
  }

  out = *created;
  return true;
}

bool CollectionRepository::listCollectionsByOwner(int64_t ownerId,
                                                  std::vector<Collection>& collections,
                                                  std::string& errorMessage) const {
  collections.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "SELECT c.id, c.name, c.description, c.owner_id, u.username, c.created_at, c.updated_at, c.is_deleted, "
      "(SELECT COUNT(1) FROM collection_posts cp JOIN posts p ON p.id = cp.post_id "
      " WHERE cp.collection_id = c.id AND p.is_deleted = 0) AS post_count "
      "FROM collections c "
      "JOIN users u ON u.id = c.owner_id "
      "WHERE c.owner_id = ? AND c.is_deleted = 0 "
      "ORDER BY c.updated_at DESC, c.id DESC;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, ownerId);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    collections.push_back(rowToCollection(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

std::optional<Collection> CollectionRepository::findById(int64_t collectionId, bool includeDeleted) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    return std::nullopt;
  }

  const char* sql =
      "SELECT c.id, c.name, c.description, c.owner_id, u.username, c.created_at, c.updated_at, c.is_deleted, "
      "(SELECT COUNT(1) FROM collection_posts cp JOIN posts p ON p.id = cp.post_id "
      " WHERE cp.collection_id = c.id AND p.is_deleted = 0) AS post_count "
      "FROM collections c "
      "JOIN users u ON u.id = c.owner_id "
      "WHERE c.id = ? AND (? = 1 OR c.is_deleted = 0) "
      "LIMIT 1;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return std::nullopt;
  }

  sqlite3_bind_int64(stmt, 1, collectionId);
  sqlite3_bind_int(stmt, 2, includeDeleted ? 1 : 0);

  std::optional<Collection> collection;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    collection = rowToCollection(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return collection;
}

bool CollectionRepository::listPostsInCollection(int64_t collectionId,
                                                 std::vector<Post>& posts,
                                                 std::string& errorMessage) const {
  posts.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted, "
      "cp.position "
      "FROM collection_posts cp "
      "JOIN posts p ON p.id = cp.post_id "
      "JOIN users u ON u.id = p.author_id "
      "WHERE cp.collection_id = ? AND p.is_deleted = 0 "
      "ORDER BY cp.position ASC;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, collectionId);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    posts.push_back(rowToPost(stmt));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool CollectionRepository::addPostToCollection(int64_t collectionId,
                                               int64_t postId,
                                               std::string& errorCode,
                                               std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorCode = "DB_ERROR";
    errorMessage = dbError;
    return false;
  }

  if (!db_.exec(db, "BEGIN;", errorMessage)) {
    errorCode = "DB_ERROR";
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* checkCollectionStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT id FROM collections WHERE id = ? AND is_deleted = 0 LIMIT 1;",
                         -1,
                         &checkCollectionStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(checkCollectionStmt, 1, collectionId);
  const int collectionRc = sqlite3_step(checkCollectionStmt);
  sqlite3_finalize(checkCollectionStmt);
  if (collectionRc != SQLITE_ROW) {
    errorCode = "COLLECTION_NOT_FOUND";
    errorMessage = "collection not found";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* checkPostStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT id FROM posts WHERE id = ? AND is_deleted = 0 LIMIT 1;",
                         -1,
                         &checkPostStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(checkPostStmt, 1, postId);
  const int postRc = sqlite3_step(checkPostStmt);
  sqlite3_finalize(checkPostStmt);
  if (postRc != SQLITE_ROW) {
    errorCode = "POST_NOT_FOUND";
    errorMessage = "post not found";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* existsStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT 1 FROM collection_posts WHERE collection_id = ? AND post_id = ? LIMIT 1;",
                         -1,
                         &existsStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(existsStmt, 1, collectionId);
  sqlite3_bind_int64(existsStmt, 2, postId);
  const int existsRc = sqlite3_step(existsStmt);
  sqlite3_finalize(existsStmt);
  if (existsRc == SQLITE_ROW) {
    errorCode = "COLLECTION_POST_EXISTS";
    errorMessage = "post already in collection";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  int nextPosition = 1;
  sqlite3_stmt* posStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT COALESCE(MAX(position), 0) + 1 FROM collection_posts WHERE collection_id = ?;",
                         -1,
                         &posStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(posStmt, 1, collectionId);
  if (sqlite3_step(posStmt) == SQLITE_ROW) {
    nextPosition = sqlite3_column_int(posStmt, 0);
  }
  sqlite3_finalize(posStmt);

  sqlite3_stmt* insertStmt = nullptr;
  if (sqlite3_prepare_v2(
          db,
          "INSERT INTO collection_posts(collection_id, post_id, position, created_at) "
          "VALUES(?, ?, ?, strftime('%Y-%m-%dT%H:%M:%SZ','now'));",
          -1,
          &insertStmt,
          nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(insertStmt, 1, collectionId);
  sqlite3_bind_int64(insertStmt, 2, postId);
  sqlite3_bind_int(insertStmt, 3, nextPosition);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(insertStmt);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(insertStmt);

  sqlite3_stmt* updateCollectionStmt = nullptr;
  if (sqlite3_prepare_v2(
          db,
          "UPDATE collections SET updated_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') WHERE id = ?;",
          -1,
          &updateCollectionStmt,
          nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(updateCollectionStmt, 1, collectionId);
  if (sqlite3_step(updateCollectionStmt) != SQLITE_DONE) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(updateCollectionStmt);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(updateCollectionStmt);

  if (!db_.exec(db, "COMMIT;", errorMessage)) {
    errorCode = "DB_ERROR";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  sqlite3_close(db);
  return true;
}

bool CollectionRepository::removePostFromCollection(int64_t collectionId,
                                                    int64_t postId,
                                                    std::string& errorCode,
                                                    std::string& errorMessage) const {
  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorCode = "DB_ERROR";
    errorMessage = dbError;
    return false;
  }

  if (!db_.exec(db, "BEGIN;", errorMessage)) {
    errorCode = "DB_ERROR";
    sqlite3_close(db);
    return false;
  }

  int removedPosition = 0;
  sqlite3_stmt* findStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT position FROM collection_posts WHERE collection_id = ? AND post_id = ? LIMIT 1;",
                         -1,
                         &findStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(findStmt, 1, collectionId);
  sqlite3_bind_int64(findStmt, 2, postId);
  if (sqlite3_step(findStmt) == SQLITE_ROW) {
    removedPosition = sqlite3_column_int(findStmt, 0);
  }
  sqlite3_finalize(findStmt);

  if (removedPosition == 0) {
    errorCode = "COLLECTION_POST_NOT_FOUND";
    errorMessage = "post is not in collection";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* deleteStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "DELETE FROM collection_posts WHERE collection_id = ? AND post_id = ?;",
                         -1,
                         &deleteStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(deleteStmt, 1, collectionId);
  sqlite3_bind_int64(deleteStmt, 2, postId);
  if (sqlite3_step(deleteStmt) != SQLITE_DONE) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(deleteStmt);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(deleteStmt);

  sqlite3_stmt* reorderStmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "UPDATE collection_posts SET position = position - 1 "
                         "WHERE collection_id = ? AND position > ?;",
                         -1,
                         &reorderStmt,
                         nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(reorderStmt, 1, collectionId);
  sqlite3_bind_int(reorderStmt, 2, removedPosition);
  if (sqlite3_step(reorderStmt) != SQLITE_DONE) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(reorderStmt);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(reorderStmt);

  sqlite3_stmt* updateCollectionStmt = nullptr;
  if (sqlite3_prepare_v2(
          db,
          "UPDATE collections SET updated_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') WHERE id = ?;",
          -1,
          &updateCollectionStmt,
          nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_int64(updateCollectionStmt, 1, collectionId);
  if (sqlite3_step(updateCollectionStmt) != SQLITE_DONE) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_finalize(updateCollectionStmt);
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(updateCollectionStmt);

  if (!db_.exec(db, "COMMIT;", errorMessage)) {
    errorCode = "DB_ERROR";
    rollback(db_, db, errorMessage);
    sqlite3_close(db);
    return false;
  }

  sqlite3_close(db);
  return true;
}

bool CollectionRepository::listCollectionsForPost(int64_t postId,
                                                  std::vector<PostCollectionMembership>& memberships,
                                                  std::string& errorMessage) const {
  memberships.clear();

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorMessage = dbError;
    return false;
  }

  const char* sql =
      "SELECT c.id, c.name, cp.position "
      "FROM collection_posts cp "
      "JOIN collections c ON c.id = cp.collection_id "
      "WHERE cp.post_id = ? AND c.is_deleted = 0 "
      "ORDER BY c.updated_at DESC, cp.position ASC;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(stmt, 1, postId);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    PostCollectionMembership item;
    item.collectionId = sqlite3_column_int64(stmt, 0);
    item.collectionName = textOrEmpty(stmt, 1);
    item.position = sqlite3_column_int(stmt, 2);
    memberships.push_back(item);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool CollectionRepository::getCollectionPostNeighbors(int64_t collectionId,
                                                      int64_t postId,
                                                      std::optional<Post>& prev,
                                                      std::optional<Post>& next,
                                                      int& currentPosition,
                                                      std::string& errorCode,
                                                      std::string& errorMessage) const {
  prev.reset();
  next.reset();
  currentPosition = 0;

  std::string dbError;
  sqlite3* db = db_.open(dbError);
  if (db == nullptr) {
    errorCode = "DB_ERROR";
    errorMessage = dbError;
    return false;
  }

  sqlite3_stmt* currentStmt = nullptr;
  const char* currentSql =
      "SELECT cp.position "
      "FROM collection_posts cp "
      "JOIN collections c ON c.id = cp.collection_id "
      "JOIN posts p ON p.id = cp.post_id "
      "WHERE cp.collection_id = ? AND cp.post_id = ? AND c.is_deleted = 0 AND p.is_deleted = 0 "
      "LIMIT 1;";

  if (sqlite3_prepare_v2(db, currentSql, -1, &currentStmt, nullptr) != SQLITE_OK) {
    errorCode = "DB_ERROR";
    errorMessage = sqlite3_errmsg(db);
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_int64(currentStmt, 1, collectionId);
  sqlite3_bind_int64(currentStmt, 2, postId);
  if (sqlite3_step(currentStmt) == SQLITE_ROW) {
    currentPosition = sqlite3_column_int(currentStmt, 0);
  }
  sqlite3_finalize(currentStmt);

  if (currentPosition == 0) {
    errorCode = "POST_NOT_IN_COLLECTION";
    errorMessage = "post is not in the specified collection";
    sqlite3_close(db);
    return false;
  }

  {
    std::string sql =
        "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted, "
        "cp.position "
        "FROM collection_posts cp "
        "JOIN posts p ON p.id = cp.post_id "
        "JOIN users u ON u.id = p.author_id "
        "WHERE cp.collection_id = ? AND cp.position < ? AND p.is_deleted = 0 "
        "ORDER BY cp.position DESC "
        "LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      errorCode = "DB_ERROR";
      errorMessage = sqlite3_errmsg(db);
      sqlite3_close(db);
      return false;
    }
    sqlite3_bind_int64(stmt, 1, collectionId);
    sqlite3_bind_int(stmt, 2, currentPosition);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      prev = rowToPost(stmt);
    }
    sqlite3_finalize(stmt);
  }

  {
    std::string sql =
        "SELECT p.id, p.title, p.content_markdown, p.author_id, u.username, p.created_at, p.updated_at, p.is_deleted, "
        "cp.position "
        "FROM collection_posts cp "
        "JOIN posts p ON p.id = cp.post_id "
        "JOIN users u ON u.id = p.author_id "
        "WHERE cp.collection_id = ? AND cp.position > ? AND p.is_deleted = 0 "
        "ORDER BY cp.position ASC "
        "LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      errorCode = "DB_ERROR";
      errorMessage = sqlite3_errmsg(db);
      sqlite3_close(db);
      return false;
    }
    sqlite3_bind_int64(stmt, 1, collectionId);
    sqlite3_bind_int(stmt, 2, currentPosition);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      next = rowToPost(stmt);
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(db);
  return true;
}

}  // namespace blog
