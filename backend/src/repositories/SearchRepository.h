#pragma once

#include <string>
#include <vector>

#include "db/Database.h"
#include "models/Post.h"

namespace blog {

class SearchRepository {
 public:
  explicit SearchRepository(const Database& db);

  bool searchPosts(const std::string& q,
                   int page,
                   int pageSize,
                   std::vector<Post>& posts,
                   int& total,
                   std::string& errorMessage) const;

 private:
  const Database& db_;
  std::string toFtsQuery(const std::string& q) const;
};

}  // namespace blog
