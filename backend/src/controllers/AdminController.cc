#include "controllers/AdminController.h"

#include "middleware/AdminMiddleware.h"
#include "middleware/AuthMiddleware.h"
#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

AdminController::AdminController(const UserRepository& userRepository, const JwtService& jwtService)
    : userRepository_(userRepository), jwtService_(jwtService) {}

Json::Value AdminController::userToJson(const User& user) const {
  Json::Value value(Json::objectValue);
  value["id"] = Json::Int64(user.id);
  value["username"] = user.username;
  value["role"] = user.role;
  value["isBanned"] = user.isBanned;
  value["createdAt"] = user.createdAt;
  return value;
}

void AdminController::listUsers(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  ApiError adminError(403, "FORBIDDEN", "admin role required");
  if (!AdminMiddleware::ensureAdmin(authUser, adminError)) {
    callback(utils::makeError(adminError, requestId));
    return;
  }

  ApiError validationError(400, "VALIDATION_ERROR", "invalid pagination");
  bool paginationOk = false;
  const auto pagination = utils::readPagination(req, 20, 50, validationError, paginationOk);
  if (!paginationOk) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::vector<User> users;
  int total = 0;
  std::string dbError;
  if (!userRepository_.listUsers(pagination.page, pagination.pageSize, users, total, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& user : users) {
    items.append(userToJson(user));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["page"] = pagination.page;
  data["pageSize"] = pagination.pageSize;
  data["total"] = total;

  callback(utils::makeSuccess(data, requestId));
}

void AdminController::updateRole(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& userId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t targetId = 0;
  if (!utils::parsePositiveInt64(userId, targetId)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid user id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  ApiError adminError(403, "FORBIDDEN", "admin role required");
  if (!AdminMiddleware::ensureAdmin(authUser, adminError)) {
    callback(utils::makeError(adminError, requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }

  const std::string role = (*body).get("role", "").asString();
  ApiError validationError(400, "VALIDATION_ERROR", "invalid role");
  if (!utils::validateRole(role, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::string dbError;
  if (!userRepository_.updateRole(targetId, role, dbError)) {
    if (dbError == "user not found") {
      callback(utils::makeError(ApiError(404, "USER_NOT_FOUND", dbError), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  const auto user = userRepository_.findById(targetId);
  if (!user.has_value()) {
    callback(utils::makeError(ApiError(404, "USER_NOT_FOUND", "user not found"), requestId));
    return;
  }

  callback(utils::makeSuccess(userToJson(*user), requestId, 200, "role updated"));
}

void AdminController::updateBan(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& userId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t targetId = 0;
  if (!utils::parsePositiveInt64(userId, targetId)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid user id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  ApiError adminError(403, "FORBIDDEN", "admin role required");
  if (!AdminMiddleware::ensureAdmin(authUser, adminError)) {
    callback(utils::makeError(adminError, requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject() || !body->isMember("isBanned") || !(*body)["isBanned"].isBool()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "isBanned(bool) is required"), requestId));
    return;
  }

  const bool isBanned = (*body)["isBanned"].asBool();
  std::string dbError;
  if (!userRepository_.updateBanStatus(targetId, isBanned, dbError)) {
    if (dbError == "user not found") {
      callback(utils::makeError(ApiError(404, "USER_NOT_FOUND", dbError), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  const auto user = userRepository_.findById(targetId);
  if (!user.has_value()) {
    callback(utils::makeError(ApiError(404, "USER_NOT_FOUND", "user not found"), requestId));
    return;
  }

  callback(utils::makeSuccess(userToJson(*user), requestId, 200, "ban status updated"));
}

}  // namespace blog
