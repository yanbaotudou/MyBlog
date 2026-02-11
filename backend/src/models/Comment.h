#pragma once

#include <cstdint>
#include <string>

namespace blog {

struct Comment {
  int64_t id = 0;
  int64_t postId = 0;
  int64_t userId = 0;
  std::string username;
  std::string content;
  std::string createdAt;
  std::string updatedAt;
  bool isDeleted = false;
};

}  // namespace blog
