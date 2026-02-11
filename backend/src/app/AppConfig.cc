#include "app/AppConfig.h"

#include <cstdlib>

namespace blog {
namespace {

std::string getenvOrDefault(const char* key, const std::string& defaultValue) {
  const char* value = std::getenv(key);
  if (value == nullptr || std::string(value).empty()) {
    return defaultValue;
  }
  return value;
}

int getenvIntOrDefault(const char* key, int defaultValue) {
  const char* value = std::getenv(key);
  if (value == nullptr) {
    return defaultValue;
  }
  try {
    return std::stoi(value);
  } catch (...) {
    return defaultValue;
  }
}

}  // namespace

bool AppConfig::isProduction() const {
  return appEnv == "production";
}

bool AppConfig::refreshCookieSecure() const {
  return isProduction();
}

AppConfig AppConfig::fromEnv() {
  AppConfig cfg;
  cfg.appEnv = getenvOrDefault("APP_ENV", "development");
  cfg.port = getenvIntOrDefault("PORT", 8080);
  cfg.dbPath = getenvOrDefault("DB_PATH", "./data/blog.db");
  cfg.migrationsDir = getenvOrDefault("MIGRATIONS_DIR", "./backend/migrations");
  cfg.jwtSecret = getenvOrDefault("JWT_SECRET", "PLEASE_CHANGE_ME_TO_A_LONG_RANDOM_SECRET");
  cfg.jwtAccessExpireMinutes = getenvIntOrDefault("JWT_ACCESS_EXPIRE_MINUTES", 15);
  cfg.refreshExpireDays = getenvIntOrDefault("REFRESH_EXPIRE_DAYS", 7);
  cfg.corsAllowOrigin = getenvOrDefault("CORS_ALLOW_ORIGIN", "http://localhost:5173");
  cfg.adminSeedUsername = getenvOrDefault("ADMIN_SEED_USERNAME", "admin");
  cfg.adminSeedPassword = getenvOrDefault("ADMIN_SEED_PASSWORD", "ChangeMe123!");
  cfg.logLevel = getenvOrDefault("LOG_LEVEL", "INFO");
  return cfg;
}

}  // namespace blog
