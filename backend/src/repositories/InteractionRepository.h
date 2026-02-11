#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/Database.h"
#include "models/Comment.h"
#include "models/Interaction.h"
#include "models/Post.h"

namespace blog {

class InteractionRepository {
 public:
  explicit InteractionRepository(const Database& db);

  bool getPostInteractionSummary(int64_t postId,
                                 const std::optional<int64_t>& currentUserId,
                                 PostInteractionSummary& summary,
                                 std::string& errorMessage) const;

  bool setLike(int64_t postId, int64_t userId, bool liked, std::string& errorMessage) const;
  bool setFavorite(int64_t postId, int64_t userId, bool favorited, std::string& errorMessage) const;
  bool listFavoritePostsByUser(int64_t userId,
                               int page,
                               int pageSize,
                               const std::string& query,
                               bool orderDesc,
                               std::vector<Post>& posts,
                               int& total,
                               std::string& errorMessage) const;

  bool listComments(int64_t postId,
                    int page,
                    int pageSize,
                    std::vector<Comment>& comments,
                    int& total,
                    std::string& errorMessage) const;

  bool createComment(int64_t postId,
                     int64_t userId,
                     const std::string& content,
                     Comment& out,
                     std::string& errorMessage) const;

  std::optional<Comment> findCommentById(int64_t commentId, bool includeDeleted = false) const;

  bool softDeleteComment(int64_t commentId, std::string& errorMessage) const;

 private:
  const Database& db_;
};

}  // namespace blog
