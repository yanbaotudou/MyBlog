#pragma once

#include <cstdint>
#include <string>

namespace blog {

struct Collection {
  int64_t id = 0;
  std::string name;
  std::string description;
  int64_t ownerId = 0;
  std::string ownerUsername;
  std::string createdAt;
  std::string updatedAt;
  bool isDeleted = false;
  int postCount = 0;
};

struct PostCollectionMembership {
  int64_t collectionId = 0;
  std::string collectionName;
  int position = 0;
};

}  // namespace blog
