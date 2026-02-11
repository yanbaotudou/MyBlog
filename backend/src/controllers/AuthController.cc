#include "controllers/AuthController.h"

#include "middleware/AuthMiddleware.h"
#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

AuthController::AuthController(const UserRepository& userRepository,
                               const PasswordService& passwordService,
                               const JwtService& jwtService,
                               RefreshTokenService& refreshTokenService)
    : userRepository_(userRepository),
      passwordService_(passwordService),
      jwtService_(jwtService),
      refreshTokenService_(refreshTokenService) {}

Json::Value AuthController::userToJson(const User& user) const {
  Json::Value value(Json::objectValue);
  value["id"] = Json::Int64(user.id);
  value["username"] = user.username;
  value["role"] = user.role;
  value["isBanned"] = user.isBanned;
  value["createdAt"] = user.createdAt;
  return value;
}

void AuthController::registerUser(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }

  const std::string username = (*body).get("username", "").asString();
  const std::string password = (*body).get("password", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid input");
  if (!utils::validateUsername(username, validationError) ||
      !utils::validatePassword(password, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::string hashError;
  const std::string passwordHash = passwordService_.hashPassword(password, hashError);
  if (passwordHash.empty()) {
    callback(utils::makeError(ApiError(500, "INTERNAL_ERROR", hashError), requestId));
    return;
  }

  User user;
  std::string errorCode;
  std::string errorMessage;
  if (!userRepository_.createUser(username, passwordHash, "user", user, errorCode, errorMessage)) {
    if (errorCode == "USERNAME_EXISTS") {
      callback(utils::makeError(ApiError(409, "USERNAME_EXISTS", "username already exists"), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, "DB_ERROR", errorMessage), requestId));
    return;
  }

  TokenPayload payload;
  payload.userId = user.id;
  payload.username = user.username;
  payload.role = user.role;

  const std::string accessToken = jwtService_.generateAccessToken(payload);

  std::string refreshRawToken;
  std::string refreshError;
  if (!refreshTokenService_.issueToken(user.id, refreshRawToken, refreshError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", refreshError), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["accessToken"] = accessToken;
  data["user"] = userToJson(user);

  auto response = utils::makeSuccess(data, requestId, 201, "registered");
  response->addHeader("Set-Cookie", refreshTokenService_.buildSetCookie(refreshRawToken, false));
  callback(response);
}

void AuthController::login(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }

  const std::string username = (*body).get("username", "").asString();
  const std::string password = (*body).get("password", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid input");
  if (!utils::validateUsername(username, validationError) ||
      !utils::validatePassword(password, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  const auto user = userRepository_.findByUsername(username);
  if (!user.has_value() || !passwordService_.verifyPassword(password, user->passwordHash)) {
    callback(utils::makeError(ApiError(401, "AUTH_INVALID_CREDENTIALS", "invalid username or password"), requestId));
    return;
  }

  if (user->isBanned) {
    callback(utils::makeError(ApiError(403, "USER_BANNED", "user is banned"), requestId));
    return;
  }

  TokenPayload payload;
  payload.userId = user->id;
  payload.username = user->username;
  payload.role = user->role;

  const std::string accessToken = jwtService_.generateAccessToken(payload);

  std::string refreshRawToken;
  std::string refreshError;
  if (!refreshTokenService_.issueToken(user->id, refreshRawToken, refreshError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", refreshError), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["accessToken"] = accessToken;
  data["user"] = userToJson(*user);

  auto response = utils::makeSuccess(data, requestId, 200, "logged in");
  response->addHeader("Set-Cookie", refreshTokenService_.buildSetCookie(refreshRawToken, false));
  callback(response);
}

void AuthController::refresh(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  const auto token = RefreshTokenService::extractRefreshToken(req);
  if (!token.has_value()) {
    callback(utils::makeError(ApiError(401, "AUTH_REQUIRED", "refresh token is required"), requestId));
    return;
  }

  int64_t userId = 0;
  std::string newRawToken;
  std::string errorCode;
  std::string errorMessage;

  if (!refreshTokenService_.rotateToken(*token, userId, newRawToken, errorCode, errorMessage)) {
    callback(utils::makeError(ApiError(401, errorCode, errorMessage), requestId));
    return;
  }

  const auto user = userRepository_.findById(userId);
  if (!user.has_value()) {
    callback(utils::makeError(ApiError(401, "AUTH_INVALID_TOKEN", "user does not exist"), requestId));
    return;
  }

  if (user->isBanned) {
    callback(utils::makeError(ApiError(403, "USER_BANNED", "user is banned"), requestId));
    return;
  }

  TokenPayload payload;
  payload.userId = user->id;
  payload.username = user->username;
  payload.role = user->role;

  Json::Value data(Json::objectValue);
  data["accessToken"] = jwtService_.generateAccessToken(payload);
  data["user"] = userToJson(*user);

  auto response = utils::makeSuccess(data, requestId, 200, "refreshed");
  response->addHeader("Set-Cookie", refreshTokenService_.buildSetCookie(newRawToken, false));
  callback(response);
}

void AuthController::logout(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  const auto token = RefreshTokenService::extractRefreshToken(req);
  if (token.has_value()) {
    std::string error;
    refreshTokenService_.revokeToken(*token, error);
  }

  Json::Value data(Json::objectValue);
  data["ok"] = true;

  auto response = utils::makeSuccess(data, requestId, 200, "logged out");
  response->addHeader("Set-Cookie", refreshTokenService_.buildSetCookie("", true));
  callback(response);
}

void AuthController::changePassword(const drogon::HttpRequestPtr& req,
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

  const std::string currentPassword = (*body).get("currentPassword", "").asString();
  const std::string newPassword = (*body).get("newPassword", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid input");
  if (!utils::validatePassword(currentPassword, validationError) ||
      !utils::validatePassword(newPassword, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  if (currentPassword == newPassword) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "new password must be different"), requestId));
    return;
  }

  const auto user = userRepository_.findById(authUser.id);
  if (!user.has_value()) {
    callback(utils::makeError(ApiError(401, "AUTH_INVALID_TOKEN", "user does not exist"), requestId));
    return;
  }

  if (!passwordService_.verifyPassword(currentPassword, user->passwordHash)) {
    callback(utils::makeError(ApiError(401, "AUTH_INVALID_CREDENTIALS", "current password is incorrect"), requestId));
    return;
  }

  if (passwordService_.verifyPassword(newPassword, user->passwordHash)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "new password must be different"), requestId));
    return;
  }

  std::string hashError;
  const std::string newPasswordHash = passwordService_.hashPassword(newPassword, hashError);
  if (newPasswordHash.empty()) {
    callback(utils::makeError(ApiError(500, "INTERNAL_ERROR", hashError), requestId));
    return;
  }

  std::string updateError;
  if (!userRepository_.updatePasswordHash(authUser.id, newPasswordHash, updateError)) {
    if (updateError == "user not found") {
      callback(utils::makeError(ApiError(404, "USER_NOT_FOUND", "user not found"), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, "DB_ERROR", updateError), requestId));
    return;
  }

  std::string revokeError;
  if (!refreshTokenService_.revokeAllByUserId(authUser.id, revokeError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", revokeError), requestId));
    return;
  }

  std::string refreshRawToken;
  std::string refreshError;
  if (!refreshTokenService_.issueToken(authUser.id, refreshRawToken, refreshError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", refreshError), requestId));
    return;
  }

  TokenPayload payload;
  payload.userId = user->id;
  payload.username = user->username;
  payload.role = user->role;

  Json::Value data(Json::objectValue);
  data["accessToken"] = jwtService_.generateAccessToken(payload);
  data["user"] = userToJson(*user);

  auto response = utils::makeSuccess(data, requestId, 200, "password changed");
  response->addHeader("Set-Cookie", refreshTokenService_.buildSetCookie(refreshRawToken, false));
  callback(response);
}

}  // namespace blog
