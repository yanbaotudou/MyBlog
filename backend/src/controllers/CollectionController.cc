#include "controllers/CollectionController.h"

#include "middleware/AuthMiddleware.h"
#include "utils/JsonResponse.h"
#include "utils/Validation.h"

namespace blog {

CollectionController::CollectionController(const CollectionRepository& collectionRepository,
                                           const PostRepository& postRepository,
                                           const UserRepository& userRepository,
                                           const JwtService& jwtService)
    : collectionRepository_(collectionRepository),
      postRepository_(postRepository),
      userRepository_(userRepository),
      jwtService_(jwtService) {}

Json::Value CollectionController::collectionToJson(const Collection& collection) const {
  Json::Value value(Json::objectValue);
  value["id"] = Json::Int64(collection.id);
  value["name"] = collection.name;
  value["description"] = collection.description;
  value["ownerId"] = Json::Int64(collection.ownerId);
  value["ownerUsername"] = collection.ownerUsername;
  value["createdAt"] = collection.createdAt;
  value["updatedAt"] = collection.updatedAt;
  value["postCount"] = collection.postCount;
  return value;
}

Json::Value CollectionController::postToJson(const Post& post) const {
  Json::Value value(Json::objectValue);
  value["id"] = Json::Int64(post.id);
  value["title"] = post.title;
  value["contentMarkdown"] = post.contentMarkdown;
  value["authorId"] = Json::Int64(post.authorId);
  value["authorUsername"] = post.authorUsername;
  value["createdAt"] = post.createdAt;
  value["updatedAt"] = post.updatedAt;
  value["isDeleted"] = post.isDeleted;
  if (post.collectionPosition > 0) {
    value["collectionPosition"] = post.collectionPosition;
  }
  return value;
}

Json::Value CollectionController::membershipToJson(const PostCollectionMembership& membership) const {
  Json::Value value(Json::objectValue);
  value["collectionId"] = Json::Int64(membership.collectionId);
  value["collectionName"] = membership.collectionName;
  value["position"] = membership.position;
  return value;
}

void CollectionController::createCollection(const drogon::HttpRequestPtr& req,
                                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject()) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "json body is required"), requestId));
    return;
  }

  const std::string name = (*body).get("name", "").asString();
  const std::string description = (*body).get("description", "").asString();

  ApiError validationError(400, "VALIDATION_ERROR", "invalid collection payload");
  if (!utils::validateCollectionName(name, validationError) ||
      !utils::validateCollectionDescription(description, validationError)) {
    callback(utils::makeError(validationError, requestId));
    return;
  }

  Collection collection;
  std::string errorCode;
  std::string errorMessage;
  if (!collectionRepository_.createCollection(
          authUser.id, name, description, collection, errorCode, errorMessage)) {
    if (errorCode == "COLLECTION_NAME_EXISTS") {
      callback(utils::makeError(ApiError(409, errorCode, errorMessage), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, errorCode, errorMessage), requestId));
    return;
  }

  callback(utils::makeSuccess(collectionToJson(collection), requestId, 201, "collection created"));
}

void CollectionController::listMyCollections(const drogon::HttpRequestPtr& req,
                                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
  const std::string requestId = utils::getRequestId(req);

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  std::vector<Collection> collections;
  std::string errorMessage;
  if (!collectionRepository_.listCollectionsByOwner(authUser.id, collections, errorMessage)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", errorMessage), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& collection : collections) {
    items.append(collectionToJson(collection));
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;

  callback(utils::makeSuccess(data, requestId));
}

void CollectionController::getCollection(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         const std::string& collectionId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t collectionIdNum = 0;
  if (!utils::parsePositiveInt64(collectionId, collectionIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid collection id"), requestId));
    return;
  }

  const auto collection = collectionRepository_.findById(collectionIdNum, false);
  if (!collection.has_value()) {
    callback(utils::makeError(ApiError(404, "COLLECTION_NOT_FOUND", "collection not found"), requestId));
    return;
  }

  std::vector<Post> posts;
  std::string errorMessage;
  if (!collectionRepository_.listPostsInCollection(collectionIdNum, posts, errorMessage)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", errorMessage), requestId));
    return;
  }

  Json::Value postItems(Json::arrayValue);
  for (const auto& post : posts) {
    postItems.append(postToJson(post));
  }

  Json::Value data(Json::objectValue);
  data["collection"] = collectionToJson(*collection);
  data["posts"] = postItems;
  data["total"] = static_cast<int>(posts.size());

  callback(utils::makeSuccess(data, requestId));
}

void CollectionController::addPostToCollection(const drogon::HttpRequestPtr& req,
                                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                               const std::string& collectionId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t collectionIdNum = 0;
  if (!utils::parsePositiveInt64(collectionId, collectionIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid collection id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto collection = collectionRepository_.findById(collectionIdNum, false);
  if (!collection.has_value()) {
    callback(utils::makeError(ApiError(404, "COLLECTION_NOT_FOUND", "collection not found"), requestId));
    return;
  }

  if (authUser.role != "admin" && authUser.id != collection->ownerId) {
    callback(utils::makeError(ApiError(403, "FORBIDDEN", "no permission to edit this collection"), requestId));
    return;
  }

  const auto body = req->getJsonObject();
  if (!body || !body->isObject() || !body->isMember("postId")) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "postId is required"), requestId));
    return;
  }

  int64_t postId = 0;
  if ((*body)["postId"].isInt64()) {
    postId = (*body)["postId"].asInt64();
  } else if ((*body)["postId"].isString()) {
    if (!utils::parsePositiveInt64((*body)["postId"].asString(), postId)) {
      callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid postId"), requestId));
      return;
    }
  } else {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid postId"), requestId));
    return;
  }

  if (postId <= 0) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid postId"), requestId));
    return;
  }

  std::string errorCode;
  std::string errorMessage;
  if (!collectionRepository_.addPostToCollection(collectionIdNum, postId, errorCode, errorMessage)) {
    if (errorCode == "COLLECTION_NOT_FOUND") {
      callback(utils::makeError(ApiError(404, errorCode, errorMessage), requestId));
      return;
    }
    if (errorCode == "POST_NOT_FOUND") {
      callback(utils::makeError(ApiError(404, errorCode, errorMessage), requestId));
      return;
    }
    if (errorCode == "COLLECTION_POST_EXISTS") {
      callback(utils::makeError(ApiError(409, errorCode, errorMessage), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, errorCode, errorMessage), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["collectionId"] = Json::Int64(collectionIdNum);
  data["postId"] = Json::Int64(postId);
  data["added"] = true;

  callback(utils::makeSuccess(data, requestId, 200, "post added to collection"));
}

void CollectionController::removePostFromCollection(const drogon::HttpRequestPtr& req,
                                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                                    const std::string& collectionId,
                                                    const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t collectionIdNum = 0;
  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(collectionId, collectionIdNum) ||
      !utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid collection id or post id"), requestId));
    return;
  }

  RequestUser authUser;
  ApiError authError(401, "AUTH_REQUIRED", "auth required");
  if (!AuthMiddleware::authenticate(req, jwtService_, userRepository_, authUser, authError)) {
    callback(utils::makeError(authError, requestId));
    return;
  }

  const auto collection = collectionRepository_.findById(collectionIdNum, false);
  if (!collection.has_value()) {
    callback(utils::makeError(ApiError(404, "COLLECTION_NOT_FOUND", "collection not found"), requestId));
    return;
  }

  if (authUser.role != "admin" && authUser.id != collection->ownerId) {
    callback(utils::makeError(ApiError(403, "FORBIDDEN", "no permission to edit this collection"), requestId));
    return;
  }

  std::string errorCode;
  std::string errorMessage;
  if (!collectionRepository_.removePostFromCollection(collectionIdNum, postIdNum, errorCode, errorMessage)) {
    if (errorCode == "COLLECTION_POST_NOT_FOUND") {
      callback(utils::makeError(ApiError(404, errorCode, errorMessage), requestId));
      return;
    }
    callback(utils::makeError(ApiError(500, errorCode, errorMessage), requestId));
    return;
  }

  Json::Value data(Json::objectValue);
  data["collectionId"] = Json::Int64(collectionIdNum);
  data["postId"] = Json::Int64(postIdNum);
  data["removed"] = true;

  callback(utils::makeSuccess(data, requestId, 200, "post removed from collection"));
}

void CollectionController::listPostCollections(const drogon::HttpRequestPtr& req,
                                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                               const std::string& postId) const {
  const std::string requestId = utils::getRequestId(req);

  int64_t postIdNum = 0;
  if (!utils::parsePositiveInt64(postId, postIdNum)) {
    callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid post id"), requestId));
    return;
  }

  const auto post = postRepository_.findById(postIdNum, false);
  if (!post.has_value()) {
    callback(utils::makeError(ApiError(404, "POST_NOT_FOUND", "post not found"), requestId));
    return;
  }

  std::vector<PostCollectionMembership> memberships;
  std::string errorMessage;
  if (!collectionRepository_.listCollectionsForPost(postIdNum, memberships, errorMessage)) {
    callback(utils::makeError(ApiError(500, "DB_ERROR", errorMessage), requestId));
    return;
  }

  Json::Value items(Json::arrayValue);
  for (const auto& membership : memberships) {
    items.append(membershipToJson(membership));
  }

  Json::Value navigation(Json::nullValue);
  const std::string collectionIdRaw = req->getParameter("collectionId");
  if (!collectionIdRaw.empty()) {
    int64_t collectionIdNum = 0;
    if (!utils::parsePositiveInt64(collectionIdRaw, collectionIdNum)) {
      callback(utils::makeError(ApiError(400, "VALIDATION_ERROR", "invalid collectionId"), requestId));
      return;
    }

    std::optional<Post> prev;
    std::optional<Post> next;
    int currentPosition = 0;
    std::string navErrorCode;
    std::string navErrorMessage;
    if (!collectionRepository_.getCollectionPostNeighbors(
            collectionIdNum, postIdNum, prev, next, currentPosition, navErrorCode, navErrorMessage)) {
      if (navErrorCode == "POST_NOT_IN_COLLECTION") {
        callback(utils::makeError(ApiError(404, navErrorCode, navErrorMessage), requestId));
        return;
      }
      callback(utils::makeError(ApiError(500, navErrorCode, navErrorMessage), requestId));
      return;
    }

    navigation = Json::Value(Json::objectValue);
    navigation["collectionId"] = Json::Int64(collectionIdNum);
    navigation["currentPosition"] = currentPosition;
    navigation["prev"] = prev.has_value() ? postToJson(*prev) : Json::Value(Json::nullValue);
    navigation["next"] = next.has_value() ? postToJson(*next) : Json::Value(Json::nullValue);
  }

  Json::Value data(Json::objectValue);
  data["items"] = items;
  data["navigation"] = navigation;

  callback(utils::makeSuccess(data, requestId));
}

}  // namespace blog
