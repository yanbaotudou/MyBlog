#include "middleware/AuthMiddleware.h"

namespace blog {
namespace {

bool parseBearerToken(const std::string& header, std::string& token) {
  constexpr const char* prefix = "Bearer ";
  if (header.size() <= 7 || header.compare(0, 7, prefix) != 0) {
    return false;
  }
  token = header.substr(7);
  return !token.empty();
}

}  // namespace

bool AuthMiddleware::authenticate(const drogon::HttpRequestPtr& req,
                                  const JwtService& jwtService,
                                  const UserRepository& userRepository,
                                  RequestUser& user,
                                  ApiError& error) {
  const std::string authHeader = req->getHeader("Authorization");
  if (authHeader.empty()) {
    error = ApiError(401, "AUTH_REQUIRED", "authorization header is required");
    return false;
  }

  std::string token;
  if (!parseBearerToken(authHeader, token)) {
    error = ApiError(401, "AUTH_INVALID_TOKEN", "authorization header must be Bearer token");
    return false;
  }

  TokenPayload payload;
  std::string errorCode;
  std::string errorMessage;
  if (!jwtService.verifyAccessToken(token, payload, errorCode, errorMessage)) {
    error = ApiError(401, errorCode, errorMessage);
    return false;
  }

  const auto dbUser = userRepository.findById(payload.userId);
  if (!dbUser.has_value()) {
    error = ApiError(401, "AUTH_INVALID_TOKEN", "user no longer exists");
    return false;
  }

  if (dbUser->isBanned) {
    error = ApiError(403, "USER_BANNED", "user is banned");
    return false;
  }

  user.id = dbUser->id;
  user.username = dbUser->username;
  user.role = dbUser->role;
  user.isBanned = dbUser->isBanned;
  return true;
}

}  // namespace blog
