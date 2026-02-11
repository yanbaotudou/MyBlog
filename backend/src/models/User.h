#pragma once

#include <cstdint>
#include <string>

namespace blog {

struct User {
  int64_t id = 0;
  std::string username;
  std::string passwordHash;
  std::string role = "user";
  bool isBanned = false;
  std::string createdAt;
};

}  // namespace blog
