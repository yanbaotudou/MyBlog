#pragma once

#include <string>

namespace blog {

class PasswordService {
 public:
  std::string hashPassword(const std::string& password, std::string& error) const;
  bool verifyPassword(const std::string& password, const std::string& encodedHash) const;
};

}  // namespace blog
