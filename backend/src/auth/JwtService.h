#pragma once

#include <cstdint>
#include <string>

#include "app/AppConfig.h"

namespace blog {

struct TokenPayload {
  int64_t userId = 0;
  std::string username;
  std::string role;
};

class JwtService {
 public:
  explicit JwtService(const AppConfig& config);

  std::string generateAccessToken(const TokenPayload& payload) const;
  bool verifyAccessToken(const std::string& token,
                         TokenPayload& payload,
                         std::string& errorCode,
                         std::string& errorMessage) const;

 private:
  std::string secret_;
  int accessExpireMinutes_;
};

}  // namespace blog
