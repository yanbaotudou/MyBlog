#pragma once

#include <drogon/drogon.h>

#include <optional>

#include "auth/JwtService.h"
#include "repositories/InteractionRepository.h"
#include "repositories/PostRepository.h"
#include "repositories/UserRepository.h"

namespace blog {

class InteractionController {
 public:
  InteractionController(const InteractionRepository& interactionRepository,
                        const PostRepository& postRepository,
                        const UserRepository& userRepository,
                        const JwtService& jwtService);

  void getPostInteractions(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& postId) const;

  void likePost(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& postId) const;

  void unlikePost(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& postId) const;

  void favoritePost(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const std::string& postId) const;

  void unfavoritePost(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      const std::string& postId) const;

  void listComments(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const std::string& postId) const;

  void createComment(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& postId) const;

  void deleteComment(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& commentId) const;

 private:
  const InteractionRepository& interactionRepository_;
  const PostRepository& postRepository_;
  const UserRepository& userRepository_;
  const JwtService& jwtService_;

  Json::Value summaryToJson(const PostInteractionSummary& summary) const;
  Json::Value commentToJson(const Comment& comment) const;

  bool tryReadOptionalAuthUserId(const drogon::HttpRequestPtr& req, std::optional<int64_t>& userId) const;
  bool ensureActivePost(int64_t postId, const std::string& requestId,
                        std::function<void(const drogon::HttpResponsePtr&)>& callback) const;
};

}  // namespace blog
