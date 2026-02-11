#include "middleware/AdminMiddleware.h"

namespace blog {

bool AdminMiddleware::ensureAdmin(const RequestUser& user, ApiError& error) {
  if (user.role != "admin") {
    error = ApiError(403, "FORBIDDEN", "admin role required");
    return false;
  }
  return true;
}

}  // namespace blog
