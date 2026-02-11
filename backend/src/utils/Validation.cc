#include "utils/Validation.h"

#include <regex>

namespace blog::utils {

bool validateUsername(const std::string& username, blog::ApiError& error) {
  if (username.size() < 3 || username.size() > 32) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "username length must be 3-32");
    return false;
  }
  static const std::regex pattern("^[A-Za-z0-9_]+$");
  if (!std::regex_match(username, pattern)) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "username only allows letters, digits and underscore");
    return false;
  }
  return true;
}

bool validatePassword(const std::string& password, blog::ApiError& error) {
  if (password.size() < 8 || password.size() > 72) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "password length must be 8-72");
    return false;
  }
  return true;
}

bool validateTitle(const std::string& title, blog::ApiError& error) {
  if (title.empty() || title.size() > 120) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "title length must be 1-120");
    return false;
  }
  return true;
}

bool validateContent(const std::string& content, blog::ApiError& error) {
  if (content.empty() || content.size() > 50000) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "content length must be 1-50000");
    return false;
  }
  return true;
}

bool validateCommentContent(const std::string& content, blog::ApiError& error) {
  if (content.empty() || content.size() > 2000) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "comment content length must be 1-2000");
    return false;
  }
  return true;
}

bool validateRole(const std::string& role, blog::ApiError& error) {
  if (role != "user" && role != "admin") {
    error = blog::ApiError(400, "VALIDATION_ERROR", "role must be user or admin");
    return false;
  }
  return true;
}

bool validateSearchQuery(const std::string& q, blog::ApiError& error) {
  if (q.empty() || q.size() > 100) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "search query length must be 1-100");
    return false;
  }
  return true;
}

bool validateCollectionName(const std::string& name, blog::ApiError& error) {
  if (name.empty() || name.size() > 80) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "collection name length must be 1-80");
    return false;
  }
  return true;
}

bool validateCollectionDescription(const std::string& description, blog::ApiError& error) {
  if (description.size() > 500) {
    error = blog::ApiError(400, "VALIDATION_ERROR", "collection description length must be <=500");
    return false;
  }
  return true;
}

bool parsePositiveInt64(const std::string& raw, int64_t& value) {
  if (raw.empty()) {
    return false;
  }
  try {
    size_t idx = 0;
    const auto parsed = std::stoll(raw, &idx, 10);
    if (idx != raw.size() || parsed <= 0) {
      return false;
    }
    value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

Pagination readPagination(const drogon::HttpRequestPtr& req,
                          int defaultPageSize,
                          int maxPageSize,
                          blog::ApiError& error,
                          bool& ok) {
  Pagination p;
  p.pageSize = defaultPageSize;

  const auto pageRaw = req->getParameter("page");
  const auto pageSizeRaw = req->getParameter("pageSize");

  if (!pageRaw.empty()) {
    try {
      p.page = std::stoi(pageRaw);
    } catch (...) {
      ok = false;
      error = blog::ApiError(400, "VALIDATION_ERROR", "page must be a positive integer");
      return p;
    }
  }

  if (!pageSizeRaw.empty()) {
    try {
      p.pageSize = std::stoi(pageSizeRaw);
    } catch (...) {
      ok = false;
      error = blog::ApiError(400, "VALIDATION_ERROR", "pageSize must be a positive integer");
      return p;
    }
  }

  if (p.page < 1 || p.pageSize < 1 || p.pageSize > maxPageSize) {
    ok = false;
    error = blog::ApiError(400,
                           "VALIDATION_ERROR",
                           "page must be >=1 and pageSize must be between 1 and " + std::to_string(maxPageSize));
    return p;
  }

  p.offset = (p.page - 1) * p.pageSize;
  ok = true;
  return p;
}

}  // namespace blog::utils
