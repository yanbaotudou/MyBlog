#pragma once

#include <drogon/HttpRequest.h>

#include <optional>
#include <string>

#include "app/AppConfig.h"
#include "db/Database.h"

namespace blog {

class RefreshTokenService {
 public:
  RefreshTokenService(const Database& db, const AppConfig& config);

  bool issueToken(int64_t userId, std::string& rawToken, std::string& error);
  bool rotateToken(const std::string& oldRawToken,
                   int64_t& userId,
                   std::string& newRawToken,
                   std::string& errorCode,
                   std::string& errorMessage);
  bool revokeToken(const std::string& rawToken, std::string& error);
  bool revokeAllByUserId(int64_t userId, std::string& error);

  std::string buildSetCookie(const std::string& rawToken, bool clear = false) const;
  static std::optional<std::string> extractRefreshToken(const drogon::HttpRequestPtr& req);

 private:
  const Database& db_;
  bool cookieSecure_;
  int refreshExpireDays_;

  std::string generateToken() const;
  std::string sha256Hex(const std::string& input) const;
};

}  // namespace blog
