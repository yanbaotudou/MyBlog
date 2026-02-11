#include "controllers/PostController.h"

#include "middleware/AuthMiddleware.h"
#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

PostController::PostController(const PostRepository& postRepository,
                               const UserRepository& userRepository,
                               const JwtService& jwtService)
    : postRepository_(postRepository), userRepository_(userRepository), jwtService_(jwtService) {}

Json::Value PostController::postToJson(const Post& post) const {
  Json::Value item(Json::objectValue);
  item["id"] = Json::Int64(post.id);
  item["title"] = post.title;
  item["contentMarkdown"] = post.contentMarkdown;
  item["authorId"] = Json::Int64(post.authorId);
  item["authorUsername"] = post.authorUsername;
  item["createdAt"] = post.createdAt;
  item["updatedAt"] = post.updatedAt;
  item["isDeleted"] = post.isDeleted;
  return item;
}

void PostController::listPosts(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  ApiError validationError(400, "VALIDATION_ERROR", "invalid pagination");
  bool paginationOk = false;
  const auto pagination = utils::readPagination(req, 10, 50, validationError, paginationOk);
  if (!paginationOk) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::vector<Post> posts;
  int total = 0;
  std::string dbError;
  if (!postRepository_.listPosts(pagination.page, pagination.pageSize, posts, total, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& post : posts) {
    items.append(postToJson(post));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["page"] = pagination.page;
  data["pageSize"] = pagination.pageSize;
  data["total"] = total;

  callback(utils::makeSuccess(data, requestId));
}

void PostController::listMyPosts(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  ApiError validationError(400, "VALIDATION_ERROR", "invalid pagination");
  bool paginationOk = false;
  const auto pagination = utils::readPagination(req, 10, 50, validationError, paginationOk);
  if (!paginationOk) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::vector<Post> posts;
  int total = 0;
  std::string dbError;
  if (!postRepository_.listPostsByAuthor(authUser.id, pagination.page, pagination.pageSize, posts, total, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& post : posts) {
    items.append(postToJson(post));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["page"] = pagination.page;
  data["pageSize"] = pagination.pageSize;
  data["total"] = total;

  callback(utils::makeSuccess(data, requestId));
}

void PostController::getPost(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t id = 0;
  if (!utils::parsePositiveInt64(postId, id)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }

  const auto post = postRepository_.findById(id, false);
  if (!post.has_value()) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return;
  }

  callback(utils::makeSuccess(postToJson(*post), requestId));
}

void PostController::createPost(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

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

  const std::string title = (*body).get("title", "").asString();
  const std::string content = (*body).get("contentMarkdown", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid post payload");
  if (!utils::validateTitle(title, validationError) || !utils::validateContent(content, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  Post created;
  std::string dbError;
  if (!postRepository_.createPost(title, content, authUser.id, created, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  callback(utils::makeSuccess(postToJson(created), requestId, 201, "post created"));
}

void PostController::updatePost(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t id = 0;
  if (!utils::parsePositiveInt64(postId, id)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto existing = postRepository_.findById(id, true);
  if (!existing.has_value() || existing->isDeleted) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return;
  }

  if (authUser.role != "admin" && authUser.id != existing->authorId) {
    callback(utils::makeError(ApiError(403, "FORBIDDEN", "no permission to edit this post"), requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }

  const std::string title = (*body).get("title", "").asString();
  const std::string content = (*body).get("contentMarkdown", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid post payload");
  if (!utils::validateTitle(title, validationError) || !utils::validateContent(content, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::string dbError;
  if (!postRepository_.updatePost(id, title, content, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  const auto updated = postRepository_.findById(id, false);
  if (!updated.has_value()) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return;
  }

  callback(utils::makeSuccess(postToJson(*updated), requestId, 200, "post updated"));
}

void PostController::deletePost(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t id = 0;
  if (!utils::parsePositiveInt64(postId, id)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto existing = postRepository_.findById(id, true);
  if (!existing.has_value() || existing->isDeleted) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return;
  }

  if (authUser.role != "admin" && authUser.id != existing->authorId) {
    callback(utils::makeError(ApiError(403, "FORBIDDEN", "no permission to delete this post"), requestId));
    return;
  }

  std::string dbError;
  if (!postRepository_.softDeletePost(id, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["deleted"] = true;
  data["id"] = Json::Int64(id);
  callback(utils::makeSuccess(data, requestId, 200, "post deleted"));
}

}  // namespace blog
