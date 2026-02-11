#pragma once

#include <drogon/drogon.h>

#include <string>

#include "auth/JwtService.h"
#include "repositories/UserRepository.h"
#include "utils/ApiError.h"

namespace blog {

struct RequestUser {
  int64_t id = 0;
  std::string username;
  std::string role;
  bool isBanned = false;
};

class AuthMiddleware {
 public:
  static bool authenticate(const drogon::HttpRequestPtr& req,
                           const JwtService& jwtService,
                           const UserRepository& userRepository,
                           RequestUser& user,
                           ApiError& error);
};

}  // namespace blog
