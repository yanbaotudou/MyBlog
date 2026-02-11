#include "auth/JwtService.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <chrono>
#include <json/json.h>
#include <sstream>
#include <vector>

namespace blog {
namespace {

std::string base64UrlEncode(const unsigned char* data, size_t len) {
  if (len == 0) {
    return "";
  }

  const int encodedLen = 4 * ((static_cast<int>(len) + 2) / 3);
  std::string encoded(encodedLen, '\0');
  const int outLen = EVP_EncodeBlock(
      reinterpret_cast<unsigned char*>(&encoded[0]), reinterpret_cast<const unsigned char*>(data), len);

  encoded.resize(outLen);
  for (char& c : encoded) {
    if (c == '+') {
      c = '-';
    } else if (c == '/') {
      c = '_';
    }
  }
  while (!encoded.empty() && encoded.back() == '=') {
    encoded.pop_back();
  }
  return encoded;
}

std::string base64UrlEncode(const std::string& input) {
  return base64UrlEncode(reinterpret_cast<const unsigned char*>(input.data()), input.size());
}

bool base64UrlDecode(const std::string& input, std::string& output) {
  std::string padded = input;
  for (char& c : padded) {
    if (c == '-') {
      c = '+';
    } else if (c == '_') {
      c = '/';
    }
  }
  while (padded.size() % 4 != 0) {
    padded.push_back('=');
  }

  std::vector<unsigned char> decoded(padded.size(), 0);
  const int len = EVP_DecodeBlock(decoded.data(),
                                  reinterpret_cast<const unsigned char*>(padded.data()),
                                  static_cast<int>(padded.size()));
  if (len < 0) {
    return false;
  }

  int realLen = len;
  if (!padded.empty() && padded[padded.size() - 1] == '=') {
    realLen--;
  }
  if (padded.size() > 1 && padded[padded.size() - 2] == '=') {
    realLen--;
  }

  output.assign(reinterpret_cast<const char*>(decoded.data()), static_cast<size_t>(realLen));
  return true;
}

std::string hmacSha256(const std::string& key, const std::string& data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digestLen = 0;

  HMAC(EVP_sha256(),
       key.data(),
       static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char*>(data.data()),
       data.size(),
       digest,
       &digestLen);

  return std::string(reinterpret_cast<char*>(digest), digestLen);
}

bool constantTimeEqual(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) {
    return false;
  }
  unsigned char diff = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    diff |= static_cast<unsigned char>(a[i] ^ b[i]);
  }
  return diff == 0;
}

bool splitJwt(const std::string& token, std::string& part1, std::string& part2, std::string& part3) {
  const size_t firstDot = token.find('.');
  if (firstDot == std::string::npos) {
    return false;
  }
  const size_t secondDot = token.find('.', firstDot + 1);
  if (secondDot == std::string::npos) {
    return false;
  }

  part1 = token.substr(0, firstDot);
  part2 = token.substr(firstDot + 1, secondDot - firstDot - 1);
  part3 = token.substr(secondDot + 1);

  return !(part1.empty() || part2.empty() || part3.empty());
}

int64_t nowEpochSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace

JwtService::JwtService(const AppConfig& config)
    : secret_(config.jwtSecret), accessExpireMinutes_(config.jwtAccessExpireMinutes) {}

std::string JwtService::generateAccessToken(const TokenPayload& payload) const {
  Json::Value header(Json::objectValue);
  header["alg"] = "HS256";
  header["typ"] = "JWT";

  const int64_t now = nowEpochSeconds();
  const int64_t exp = now + static_cast<int64_t>(accessExpireMinutes_ * 60);

  Json::Value body(Json::objectValue);
  body["sub"] = Json::Int64(payload.userId);
  body["username"] = payload.username;
  body["role"] = payload.role;
  body["iat"] = Json::Int64(now);
  body["exp"] = Json::Int64(exp);

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";

  const std::string headerRaw = Json::writeString(builder, header);
  const std::string bodyRaw = Json::writeString(builder, body);

  const std::string headerPart = base64UrlEncode(headerRaw);
  const std::string bodyPart = base64UrlEncode(bodyRaw);
  const std::string signingInput = headerPart + "." + bodyPart;

  const std::string sig = hmacSha256(secret_, signingInput);
  const std::string sigPart = base64UrlEncode(reinterpret_cast<const unsigned char*>(sig.data()), sig.size());

  return signingInput + "." + sigPart;
}

bool JwtService::verifyAccessToken(const std::string& token,
                                   TokenPayload& payload,
                                   std::string& errorCode,
                                   std::string& errorMessage) const {
  std::string headerPart;
  std::string bodyPart;
  std::string sigPart;
  if (!splitJwt(token, headerPart, bodyPart, sigPart)) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "invalid token format";
    return false;
  }

  const std::string signingInput = headerPart + "." + bodyPart;
  const std::string expectedSig = hmacSha256(secret_, signingInput);
  const std::string expectedSigPart =
      base64UrlEncode(reinterpret_cast<const unsigned char*>(expectedSig.data()), expectedSig.size());

  if (!constantTimeEqual(expectedSigPart, sigPart)) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "token signature mismatch";
    return false;
  }

  std::string bodyRaw;
  if (!base64UrlDecode(bodyPart, bodyRaw)) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "token payload decode failed";
    return false;
  }

  Json::CharReaderBuilder readerBuilder;
  std::string parseErrors;
  Json::Value body;
  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  const bool ok = reader->parse(
      bodyRaw.data(), bodyRaw.data() + bodyRaw.size(), &body, &parseErrors);

  if (!ok) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "token payload parse failed";
    return false;
  }

  if (!body.isMember("sub") || !body["sub"].isInt64() || !body.isMember("exp") || !body["exp"].isInt64() ||
      !body.isMember("username") || !body["username"].isString() || !body.isMember("role") ||
      !body["role"].isString()) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "token payload missing required fields";
    return false;
  }

  const int64_t exp = body["exp"].asInt64();
  if (nowEpochSeconds() >= exp) {
    errorCode = "AUTH_INVALID_TOKEN";
    errorMessage = "token expired";
    return false;
  }

  payload.userId = body["sub"].asInt64();
  payload.username = body["username"].asString();
  payload.role = body["role"].asString();
  return true;
}

}  // namespace blog
