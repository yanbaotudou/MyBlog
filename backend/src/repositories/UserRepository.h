#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/Database.h"
#include "models/User.h"

namespace blog {

class UserRepository {
 public:
  explicit UserRepository(const Database& db);

  std::optional<User> findByUsername(const std::string& username) const;
  std::optional<User> findById(int64_t id) const;

  bool createUser(const std::string& username,
                  const std::string& passwordHash,
                  const std::string& role,
                  User& out,
                  std::string& errorCode,
                  std::string& errorMessage) const;

  bool ensureDefaultAdmin(const std::string& username,
                          const std::string& passwordHash,
                          bool& created,
                          std::string& errorMessage) const;

  bool listUsers(int page,
                 int pageSize,
                 std::vector<User>& users,
                 int& total,
                 std::string& errorMessage) const;

  bool updateRole(int64_t userId, const std::string& role, std::string& errorMessage) const;
  bool updateBanStatus(int64_t userId, bool isBanned, std::string& errorMessage) const;
  bool updatePasswordHash(int64_t userId, const std::string& passwordHash, std::string& errorMessage) const;

 private:
  const Database& db_;
};

}  // namespace blog
