#pragma once

#include <drogon/drogon.h>

#include "auth/JwtService.h"
#include "auth/PasswordService.h"
#include "auth/RefreshTokenService.h"
#include "repositories/UserRepository.h"

namespace blog {

class AuthController {
 public:
  AuthController(const UserRepository& userRepository,
                 const PasswordService& passwordService,
                 const JwtService& jwtService,
                 RefreshTokenService& refreshTokenService);

  void registerUser(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void login(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void refresh(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void logout(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

  void changePassword(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

 private:
  const UserRepository& userRepository_;
  const PasswordService& passwordService_;
  const JwtService& jwtService_;
  RefreshTokenService& refreshTokenService_;

  Json::Value userToJson(const User& user) const;
};

}  // namespace blog
