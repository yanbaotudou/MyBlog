#pragma once

#include <drogon/drogon.h>

#include "auth/JwtService.h"
#include "repositories/UserRepository.h"

namespace blog {

class AdminController {
 public:
  AdminController(const UserRepository& userRepository, const JwtService& jwtService);

  void listUsers(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void updateRole(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& userId) const;

  void updateBan(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 const std::string& userId) const;

 private:
  const UserRepository& userRepository_;
  const JwtService& jwtService_;

  Json::Value userToJson(const User& user) const;
};

}  // namespace blog
