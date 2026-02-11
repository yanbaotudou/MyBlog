#pragma once

#include <json/json.h>

#include <string>

namespace blog {

struct ApiError {
  int httpStatus;
  std::string code;
  std::string message;
  Json::Value details;

  ApiError(int status,
           std::string errorCode,
           std::string errorMessage,
           Json::Value errorDetails = Json::Value(Json::objectValue));
};

}  // namespace blog
