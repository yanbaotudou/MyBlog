#pragma once

#include <drogon/drogon.h>

#include <string>

#include "utils/ApiError.h"

namespace blog::utils {

inline std::string getRequestId(const drogon::HttpRequestPtr& req) {
  const auto incoming = req->getHeader("X-Request-Id");
  if (!incoming.empty()) {
    return incoming;
  }
  return drogon::utils::getUuid();
}

inline drogon::HttpResponsePtr makeSuccess(const Json::Value& data,
                                           const std::string& requestId,
                                           int status = 200,
                                           const std::string& message = "success") {
  Json::Value body(Json::objectValue);
  body["code"] = "OK";
  body["message"] = message;
  body["data"] = data;
  body["requestId"] = requestId;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status));
  return resp;
}

inline drogon::HttpResponsePtr makeError(const blog::ApiError& error,
                                         const std::string& requestId) {
  Json::Value body(Json::objectValue);
  body["code"] = error.code;
  body["message"] = error.message;
  body["details"] = error.details;
  body["requestId"] = requestId;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.httpStatus));
  return resp;
}

}  // namespace blog::utils
