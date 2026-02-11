#pragma once

namespace blog {

struct PostInteractionSummary {
  int likeCount = 0;
  int favoriteCount = 0;
  int commentCount = 0;
  bool likedByMe = false;
  bool favoritedByMe = false;
};

}  // namespace blog
