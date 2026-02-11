#pragma once

#include <drogon/HttpRequest.h>

#include <cstdint>
#include <string>

#include "utils/ApiError.h"

namespace blog::utils {

struct Pagination {
  int page = 1;
  int pageSize = 10;
  int offset = 0;
};

bool validateUsername(const std::string& username, blog::ApiError& error);
bool validatePassword(const std::string& password, blog::ApiError& error);
bool validateTitle(const std::string& title, blog::ApiError& error);
bool validateContent(const std::string& content, blog::ApiError& error);
bool validateCommentContent(const std::string& content, blog::ApiError& error);
bool validateRole(const std::string& role, blog::ApiError& error);
bool validateSearchQuery(const std::string& q, blog::ApiError& error);
bool validateCollectionName(const std::string& name, blog::ApiError& error);
bool validateCollectionDescription(const std::string& description, blog::ApiError& error);
bool parsePositiveInt64(const std::string& raw, int64_t& value);
Pagination readPagination(const drogon::HttpRequestPtr& req,
                          int defaultPageSize,
                          int maxPageSize,
                          blog::ApiError& error,
                          bool& ok);

}  // namespace blog::utils
