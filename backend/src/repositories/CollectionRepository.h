#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/Database.h"
#include "models/Collection.h"
#include "models/Post.h"

namespace blog {

class CollectionRepository {
 public:
  explicit CollectionRepository(const Database& db);

  bool createCollection(int64_t ownerId,
                        const std::string& name,
                        const std::string& description,
                        Collection& out,
                        std::string& errorCode,
                        std::string& errorMessage) const;

  bool listCollectionsByOwner(int64_t ownerId,
                              std::vector<Collection>& collections,
                              std::string& errorMessage) const;

  std::optional<Collection> findById(int64_t collectionId, bool includeDeleted = false) const;

  bool listPostsInCollection(int64_t collectionId,
                             std::vector<Post>& posts,
                             std::string& errorMessage) const;

  bool addPostToCollection(int64_t collectionId,
                           int64_t postId,
                           std::string& errorCode,
                           std::string& errorMessage) const;

  bool removePostFromCollection(int64_t collectionId,
                                int64_t postId,
                                std::string& errorCode,
                                std::string& errorMessage) const;

  bool listCollectionsForPost(int64_t postId,
                              std::vector<PostCollectionMembership>& memberships,
                              std::string& errorMessage) const;

  bool getCollectionPostNeighbors(int64_t collectionId,
                                  int64_t postId,
                                  std::optional<Post>& prev,
                                  std::optional<Post>& next,
                                  int& currentPosition,
                                  std::string& errorCode,
                                  std::string& errorMessage) const;

 private:
  const Database& db_;
};

}  // namespace blog
