#pragma once

#include <cstdint>
#include <string>

namespace blog {

struct Post {
  int64_t id = 0;
  std::string title;
  std::string contentMarkdown;
  int64_t authorId = 0;
  std::string authorUsername;
  std::string createdAt;
  std::string updatedAt;
  std::string favoritedAt;
  bool isDeleted = false;
  int collectionPosition = 0;
};

}  // namespace blog
