#include "auth/PasswordService.h"

#include <argon2.h>
#include <openssl/rand.h>

#include <cstring>
#include <vector>

namespace blog {

std::string PasswordService::hashPassword(const std::string& password, std::string& error) const {
  constexpr uint32_t tCost = 2;
  constexpr uint32_t mCost = 1 << 16;
  constexpr uint32_t parallelism = 1;
  constexpr size_t hashLen = 32;
  constexpr size_t saltLen = 16;

  std::vector<uint8_t> salt(saltLen);
  if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
    error = "failed to generate random salt";
    return "";
  }

  const size_t encodedLen = argon2_encodedlen(tCost, mCost, parallelism, saltLen, hashLen, Argon2_id);
  std::string encoded(encodedLen, '\0');

  const int rc = argon2id_hash_encoded(tCost,
                                       mCost,
                                       parallelism,
                                       password.c_str(),
                                       password.size(),
                                       salt.data(),
                                       salt.size(),
                                       hashLen,
                                       encoded.data(),
                                       encoded.size());
  if (rc != ARGON2_OK) {
    error = argon2_error_message(rc);
    return "";
  }

  encoded.resize(std::strlen(encoded.c_str()));
  return encoded;
}

bool PasswordService::verifyPassword(const std::string& password, const std::string& encodedHash) const {
  const int rc = argon2id_verify(encodedHash.c_str(), password.c_str(), password.size());
  return rc == ARGON2_OK;
}

}  // namespace blog
