#pragma once

#include "middleware/AuthMiddleware.h"
#include "utils/ApiError.h"

namespace blog {

class AdminMiddleware {
 public:
  static bool ensureAdmin(const RequestUser& user, ApiError& error);
};

}  // namespace blog
