#include "controllers/SearchController.h"

#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

SearchController::SearchController(const SearchRepository& searchRepository)
    : searchRepository_(searchRepository) {}

Json::Value SearchController::postToJson(const Post& post) const {
  Json::Value item(Json::objectValue);
  item["id"] = Json::Int64(post.id);
  item["title"] = post.title;
  item["contentMarkdown"] = post.contentMarkdown;
  item["authorId"] = Json::Int64(post.authorId);
  item["authorUsername"] = post.authorUsername;
  item["createdAt"] = post.createdAt;
  item["updatedAt"] = post.updatedAt;
  return item;
}

void SearchController::search(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  const std::string q = req->getParameter("q");
  ApiError validationError(400, "VALIDATION_ERROR", "invalid query");

  if (!utils::validateSearchQuery(q, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  bool paginationOk = false;
  const auto pagination = utils::readPagination(req, 10, 50, validationError, paginationOk);
  if (!paginationOk) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  std::vector<Post> posts;
  int total = 0;
  std::string dbError;
  if (!searchRepository_.searchPosts(q, pagination.page, pagination.pageSize, posts, total, dbError)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", dbError), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& post : posts) {
    items.append(postToJson(post));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["q"] = q;
  data["page"] = pagination.page;
  data["pageSize"] = pagination.pageSize;
  data["total"] = total;

  callback(utils::makeSuccess(data, requestId));
}

}  // namespace blog
