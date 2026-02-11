#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/Database.h"
#include "models/Post.h"

namespace blog {

class PostRepository {
 public:
  explicit PostRepository(const Database& db);

  bool listPosts(int page,
                 int pageSize,
                 std::vector<Post>& posts,
                 int& total,
                 std::string& errorMessage) const;

  bool listPostsByAuthor(int64_t authorId,
                         int page,
                         int pageSize,
                         std::vector<Post>& posts,
                         int& total,
                         std::string& errorMessage) const;

  std::optional<Post> findById(int64_t id, bool includeDeleted = false) const;

  bool createPost(const std::string& title,
                  const std::string& contentMarkdown,
                  int64_t authorId,
                  Post& out,
                  std::string& errorMessage) const;

  bool updatePost(int64_t id,
                  const std::string& title,
                  const std::string& contentMarkdown,
                  std::string& errorMessage) const;

  bool softDeletePost(int64_t id, std::string& errorMessage) const;

 private:
  const Database& db_;
};

}  // namespace blog
