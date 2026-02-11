#pragma once

#include <drogon/drogon.h>

#include "repositories/SearchRepository.h"

namespace blog {

class SearchController {
 public:
  explicit SearchController(const SearchRepository& searchRepository);

  void search(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

 private:
  const SearchRepository& searchRepository_;

  Json::Value postToJson(const Post& post) const;
};

}  // namespace blog
