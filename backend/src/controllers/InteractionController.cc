#include "controllers/InteractionController.h"

#include "middleware/AuthMiddleware.h"
#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

InteractionController::InteractionController(const InteractionRepository& interactionRepository,
                                             const PostRepository& postRepository,
                                             const UserRepository& userRepository,
                                             const JwtService& jwtService)
    : interactionRepository_(interactionRepository),
      postRepository_(postRepository),
      userRepository_(userRepository),
      jwtService_(jwtService) {}

Json::Value InteractionController::summaryToJson(const PostInteractionSummary& summary) const {
  Json::Value value(Json::objectValue);
  value["likeCount"] = summary.likeCount;
  value["favoriteCount"] = summary.favoriteCount;
  value["commentCount"] = summary.commentCount;
  value["likedByMe"] = summary.likedByMe;
  value["favoritedByMe"] = summary.favoritedByMe;
  return value;
}

Json::Value InteractionController::commentToJson(const Comment& comment) const {
  Json::Value value(Json::objectValue);
  value["id"] = Json::Int64(comment.id);
  value["postId"] = Json::Int64(comment.postId);
  value["userId"] = Json::Int64(comment.userId);
  value["username"] = comment.username;
  value["content"] = comment.content;
  value["createdAt"] = comment.createdAt;
  value["updatedAt"] = comment.updatedAt;
  value["isDeleted"] = comment.isDeleted;
  return value;
}

bool InteractionController::tryReadOptionalAuthUserId(const drogon::HttpRequestPtr& req,
                                                      std::optional<int64_t>& userId) const {
  userId.reset();
  const std::string auth = req->getHeader("Authorization");
  if (auth.rfind("Bearer ", 0) != 0) {
    return true;
  }

  const std::string token = auth.substr(7);
  TokenPayload payload;
  std::string errorCode;
  std::string errorMessage;
  if (!jwtService_.verifyAccessToken(token, payload, errorCode, errorMessage)) {
    return true;
  }

  const auto user = userRepository_.findById(payload.userId);
  if (!user.has_value() || user->isBanned) {
    return true;
  }
  userId = user->id;
  return true;
}

bool InteractionController::ensureActivePost(
    int64_t postId, const std::string& requestId, std::function<void(const drogon::HttpResponsePtr&)>& callback) const {
  const auto post = postRepository_.findById(postId, false);
  if (!post.has_value() || post->isDeleted) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return false;
  }
  return true;
}

void InteractionController::getPostInteractions(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }

  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  std::optional<int64_t> currentUserId;
  tryReadOptionalAuthUserId(req, currentUserId);

  PostInteractionSummary summary;
  std::string errorMessage;
  if (!interactionRepository_.getPostInteractionSummary(postIdNum, currentUserId, summary, errorMessage)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", errorMessage), requestId));
    return;
  }
  callback(utils::makeSuccess(summaryToJson(summary), requestId));
}

void InteractionController::likePost(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                     const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  std::string dbError;
  if (!interactionRepository_.setLike(postIdNum, authUser.id, true, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  PostInteractionSummary summary;
  if (!interactionRepository_.getPostInteractionSummary(postIdNum, authUser.id, summary, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }
  callback(utils::makeSuccess(summaryToJson(summary), requestId, 200, "liked"));
}

void InteractionController::unlikePost(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                       const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  std::string dbError;
  if (!interactionRepository_.setLike(postIdNum, authUser.id, false, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  PostInteractionSummary summary;
  if (!interactionRepository_.getPostInteractionSummary(postIdNum, authUser.id, summary, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }
  callback(utils::makeSuccess(summaryToJson(summary), requestId, 200, "unliked"));
}

void InteractionController::favoritePost(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  std::string dbError;
  if (!interactionRepository_.setFavorite(postIdNum, authUser.id, true, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  PostInteractionSummary summary;
  if (!interactionRepository_.getPostInteractionSummary(postIdNum, authUser.id, summary, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }
  callback(utils::makeSuccess(summaryToJson(summary), requestId, 200, "favorited"));
}

void InteractionController::unfavoritePost(const drogon::HttpRequestPtr& req,
                                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                           const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  std::string dbError;
  if (!interactionRepository_.setFavorite(postIdNum, authUser.id, false, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  PostInteractionSummary summary;
  if (!interactionRepository_.getPostInteractionSummary(postIdNum, authUser.id, summary, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }
  callback(utils::makeSuccess(summaryToJson(summary), requestId, 200, "unfavorited"));
}

void InteractionController::listComments(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  ApiError validationError(400, "VALIDATION_ERROR", "invalid pagination");
  bool paginationOk = false;
  const auto pagination = utils::readPagination(req, 10, 50, validationError, paginationOk);
  if (!paginationOk) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::vector<Comment> comments;
  int total = 0;
  std::string dbError;
  if (!interactionRepository_.listComments(postIdNum, pagination.page, pagination.pageSize, comments, total, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& comment : comments) {
    items.append(commentToJson(comment));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["page"] = pagination.page;
  data["pageSize"] = pagination.pageSize;
  data["total"] = total;
  callback(utils::makeSuccess(data, requestId));
}

void InteractionController::createComment(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }
  if (!ensureActivePost(postIdNum, requestId, callback)) {
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }
  const std::string content = (*body).get("content", "").asString();
  ApiError validationError(400, "VALIDATION_ERROR", "invalid comment payload");
  if (!utils::validateCommentContent(content, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  Comment created;
  std::string dbError;
  if (!interactionRepository_.createComment(postIdNum, authUser.id, content, created, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }
  callback(utils::makeSuccess(commentToJson(created), requestId, 201, "comment created"));
}

void InteractionController::deleteComment(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                          const std::string& commentId) const {
  const std::string requestId = utils::getRequestId(req);
  int64_t commentIdNum = 0;
  if (!utils::parsePositiveInt64(commentId, commentIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid comment id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto existing = interactionRepository_.findCommentById(commentIdNum, true);
  if (!existing.has_value() || existing->isDeleted) {
    callback(utils::makeError(ApiError(404, "COMMENT_NOT_FOUND", "comment not found"), requestId));
    return;
  }
  if (authUser.role != "admin" && authUser.id != existing->userId) {
    callback(utils::makeError(ApiError(403, "FORBIDDEN", "no permission to delete this comment"), requestId));
    return;
  }

  std::string dbError;
  if (!interactionRepository_.softDeleteComment(commentIdNum, dbError)) {
    if (dbError == "comment not found") {
      callback(utils::makeError(ApiError(404, "COMMENT_NOT_FOUND", "comment not found"), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["deleted"] = true;
  data["id"] = Json::Int64(commentIdNum);
  callback(utils::makeSuccess(data, requestId, 200, "comment deleted"));
}

}  // namespace blog
