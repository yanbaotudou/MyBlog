#include "utils/ApiError.h"

namespace blog {

ApiError::ApiError(int status,
                   std::string errorCode,
                   std::string errorMessage,
                   Json::Value errorDetails)
    : httpStatus(status),
      code(std::move(errorCode)),
      message(std::move(errorMessage)),
      details(std::move(errorDetails)) {}

}  // namespace blog
