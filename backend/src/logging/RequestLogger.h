#pragma once

#include <drogon/drogon.h>

namespace blog::logging {

inline void registerRequestLogger() {
  drogon::app().registerPreRoutingAdvice(
      [](const drogon::HttpRequestPtr& req,
         drogon::AdviceCallback&&,
         drogon::AdviceChainCallback&& chainCallback) {
        LOG_INFO << "incoming request" << " method=" << req->methodString() << " path=" << req->path();
        chainCallback();
      });

  drogon::app().registerPostHandlingAdvice(
      [](const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
        LOG_INFO << "request complete"
                 << " method=" << req->methodString() << " path=" << req->path()
                 << " status=" << static_cast<int>(resp->statusCode());
      });
}

}  // namespace blog::logging
