#pragma once

#include <string>

namespace blog {

struct AppConfig {
  std::string appEnv;
  int port;
  std::string dbPath;
  std::string migrationsDir;
  std::string jwtSecret;
  int jwtAccessExpireMinutes;
  int refreshExpireDays;
  std::string corsAllowOrigin;
  std::string adminSeedUsername;
  std::string adminSeedPassword;
  std::string logLevel;

  bool isProduction() const;
  bool refreshCookieSecure() const;

  static AppConfig fromEnv();
};

}  // namespace blog
