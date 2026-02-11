#pragma once

#include <drogon/drogon.h>

#include "auth/JwtService.h"
#include "repositories/PostRepository.h"
#include "repositories/UserRepository.h"

namespace blog {

class PostController {
 public:
  PostController(const PostRepository& postRepository,
                 const UserRepository& userRepository,
                 const JwtService& jwtService);

  void listPosts(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void getPost(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& postId) const;

  void createPost(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void updatePost(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& postId) const;

  void deletePost(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& postId) const;

 private:
  const PostRepository& postRepository_;
  const UserRepository& userRepository_;
  const JwtService& jwtService_;

  Json::Value postToJson(const Post& post) const;
};

}  // namespace blog
