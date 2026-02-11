#pragma once

#include <drogon/drogon.h>

#include "auth/JwtService.h"
#include "repositories/CollectionRepository.h"
#include "repositories/PostRepository.h"
#include "repositories/UserRepository.h"

namespace blog {

class CollectionController {
 public:
  CollectionController(const CollectionRepository& collectionRepository,
                       const PostRepository& postRepository,
                       const UserRepository& userRepository,
                       const JwtService& jwtService);

  void createCollection(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void listMyCollections(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void getCollection(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& collectionId) const;

  void addPostToCollection(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& collectionId) const;

  void removePostFromCollection(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& collectionId,
                                const std::string& postId) const;

  void listPostCollections(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& postId) const;

 private:
  const CollectionRepository& collectionRepository_;
  const PostRepository& postRepository_;
  const UserRepository& userRepository_;
  const JwtService& jwtService_;

  Json::Value collectionToJson(const Collection& collection) const;
  Json::Value postToJson(const Post& post) const;
  Json::Value membershipToJson(const PostCollectionMembership& membership) const;
};

}  // namespace blog
