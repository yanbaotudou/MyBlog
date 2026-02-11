#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>

#include <filesystem>

#include "app/AppConfig.h"
#include "auth/JwtService.h"
#include "auth/PasswordService.h"
#include "auth/RefreshTokenService.h"
#include "controllers/AdminController.h"
#include "controllers/AuthController.h"
#include "controllers/CollectionController.h"
#include "controllers/InteractionController.h"
#include "controllers/PostController.h"
#include "controllers/SearchController.h"
#include "db/Database.h"
#include "logging/RequestLogger.h"
#include "repositories/PostRepository.h"
#include "repositories/SearchRepository.h"
#include "repositories/CollectionRepository.h"
#include "repositories/InteractionRepository.h"
#include "repositories/UserRepository.h"
#include "utils/Validation.h"

namespace {

void applyLogLevel(const std::string& level) {
  if (level == "TRACE") {
    trantor::Logger::setLogLevel(trantor::Logger::kTrace);
  } else if (level == "DEBUG") {
    trantor::Logger::setLogLevel(trantor::Logger::kDebug);
  } else if (level == "WARN") {
    trantor::Logger::setLogLevel(trantor::Logger::kWarn);
  } else if (level == "ERROR") {
    trantor::Logger::setLogLevel(trantor::Logger::kError);
  } else {
    trantor::Logger::setLogLevel(trantor::Logger::kInfo);
  }
}

void addCorsHeaders(const blog::AppConfig& config, const drogon::HttpResponsePtr& resp) {
  resp->addHeader("Access-Control-Allow-Origin", config.corsAllowOrigin);
  resp->addHeader("Access-Control-Allow-Credentials", "true");
  resp->addHeader("Access-Control-Allow-Headers", "Authorization, Content-Type, X-Request-Id");
  resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

bool runSetup(const blog::AppConfig& config,
              const blog::Database& db,
              const blog::PasswordService& passwordService) {
  std::string dirError;
  if (!db.ensureParentDir(dirError)) {
    LOG_ERROR << "failed to create db directory: " << dirError;
    return false;
  }

  std::vector<std::string> applied;
  std::string migrationError;
  if (!runMigrations(db, config.migrationsDir, applied, migrationError)) {
    LOG_ERROR << "failed to run migrations: " << migrationError;
    return false;
  }

  for (const auto& file : applied) {
    LOG_INFO << "applied migration: " << file;
  }

  blog::ApiError passwordValidation(400, "VALIDATION_ERROR", "invalid password");
  if (!blog::utils::validatePassword(config.adminSeedPassword, passwordValidation)) {
    LOG_ERROR << "ADMIN_SEED_PASSWORD is invalid: " << passwordValidation.message;
    return false;
  }

  std::string hashError;
  const std::string adminHash = passwordService.hashPassword(config.adminSeedPassword, hashError);
  if (adminHash.empty()) {
    LOG_ERROR << "failed to hash ADMIN_SEED_PASSWORD: " << hashError;
    return false;
  }

  blog::UserRepository userRepository(db);
  bool created = false;
  std::string seedError;
  if (!userRepository.ensureDefaultAdmin(config.adminSeedUsername, adminHash, created, seedError)) {
    LOG_ERROR << "failed to ensure default admin: " << seedError;
    return false;
  }

  if (created) {
    LOG_WARN << "default admin account created: username=" << config.adminSeedUsername
             << " . Please change password immediately after first login.";
  } else {
    LOG_INFO << "default admin already exists, skip seeding";
  }

  return true;
}

}  // namespace

int main() {
  const blog::AppConfig config = blog::AppConfig::fromEnv();
  applyLogLevel(config.logLevel);

  LOG_INFO << "starting Study Blog API on port " << config.port;
  LOG_INFO << "db path: " << config.dbPath;

  const blog::Database db(config.dbPath);
  const blog::PasswordService passwordService;

  if (!runSetup(config, db, passwordService)) {
    return 1;
  }

  const blog::UserRepository userRepository(db);
  const blog::PostRepository postRepository(db);
  const blog::SearchRepository searchRepository(db);
  const blog::CollectionRepository collectionRepository(db);
  const blog::InteractionRepository interactionRepository(db);

  const blog::JwtService jwtService(config);
  blog::RefreshTokenService refreshTokenService(db, config);

  const blog::AuthController authController(userRepository, passwordService, jwtService, refreshTokenService);
  const blog::PostController postController(postRepository, userRepository, jwtService);
  const blog::SearchController searchController(searchRepository);
  const blog::AdminController adminController(userRepository, jwtService);
  const blog::CollectionController collectionController(
      collectionRepository, postRepository, userRepository, jwtService);
  const blog::InteractionController interactionController(
      interactionRepository, postRepository, userRepository, jwtService);

  blog::logging::registerRequestLogger();

  drogon::app().registerPreRoutingAdvice(
      [&config](const drogon::HttpRequestPtr& req,
                drogon::AdviceCallback&& callback,
                drogon::AdviceChainCallback&& chainCallback) {
        if (req->method() == drogon::Options) {
          auto resp = drogon::HttpResponse::newHttpResponse();
          resp->setStatusCode(drogon::k204NoContent);
          addCorsHeaders(config, resp);
          callback(resp);
          return;
        }
        chainCallback();
      });

  drogon::app().registerPostHandlingAdvice(
      [&config](const drogon::HttpRequestPtr&, const drogon::HttpResponsePtr& resp) {
        addCorsHeaders(config, resp);
      });

  drogon::app().registerHandler(
      "/api/auth/register",
      [&authController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        authController.registerUser(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/login",
      [&authController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        authController.login(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/refresh",
      [&authController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        authController.refresh(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/logout",
      [&authController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        authController.logout(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/change-password",
      [&authController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        authController.changePassword(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/posts",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        postController.listPosts(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        postController.createPost(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/posts/mine",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        postController.listMyPosts(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/me/posts",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        postController.listMyPosts(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts/{1}",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id) { postController.getPost(req, std::move(callback), id); },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts/{1}",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id) { postController.updatePost(req, std::move(callback), id); },
      {drogon::Put});

  drogon::app().registerHandler(
      "/api/posts/{1}",
      [&postController](const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id) { postController.deletePost(req, std::move(callback), id); },
      {drogon::Delete});

  drogon::app().registerHandler(
      "/api/posts/{1}/interactions",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.getPostInteractions(req, std::move(callback), postId);
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts/{1}/like",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.likePost(req, std::move(callback), postId);
      },
      {drogon::Put});

  drogon::app().registerHandler(
      "/api/posts/{1}/like",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.unlikePost(req, std::move(callback), postId);
      },
      {drogon::Delete});

  drogon::app().registerHandler(
      "/api/posts/{1}/favorite",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.favoritePost(req, std::move(callback), postId);
      },
      {drogon::Put});

  drogon::app().registerHandler(
      "/api/posts/{1}/favorite",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.unfavoritePost(req, std::move(callback), postId);
      },
      {drogon::Delete});

  drogon::app().registerHandler(
      "/api/me/favorites",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        interactionController.listMyFavorites(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts/{1}/comments",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.listComments(req, std::move(callback), postId);
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/posts/{1}/comments",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& postId) {
        interactionController.createComment(req, std::move(callback), postId);
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/comments/{1}",
      [&interactionController](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& commentId) {
        interactionController.deleteComment(req, std::move(callback), commentId);
      },
      {drogon::Delete});

  drogon::app().registerHandler(
      "/api/search",
      [&searchController](const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        searchController.search(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/admin/users",
      [&adminController](const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        adminController.listUsers(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/admin/users/{1}/role",
      [&adminController](const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id) { adminController.updateRole(req, std::move(callback), id); },
      {drogon::Put});

  drogon::app().registerHandler(
      "/api/admin/users/{1}/ban",
      [&adminController](const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id) { adminController.updateBan(req, std::move(callback), id); },
      {drogon::Put});

  drogon::app().registerHandler(
      "/api/collections",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        collectionController.createCollection(req, std::move(callback));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/collections/mine",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        collectionController.listMyCollections(req, std::move(callback));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/collections/{1}",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& collectionId) {
        collectionController.getCollection(req, std::move(callback), collectionId);
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/collections/{1}/posts",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& collectionId) {
        collectionController.addPostToCollection(req, std::move(callback), collectionId);
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/collections/{1}/posts/{2}",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& collectionId,
                              const std::string& postId) {
        collectionController.removePostFromCollection(req, std::move(callback), collectionId, postId);
      },
      {drogon::Delete});

  drogon::app().registerHandler(
      "/api/posts/{1}/collections",
      [&collectionController](const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& postId) {
        collectionController.listPostCollections(req, std::move(callback), postId);
      },
      {drogon::Get});

  drogon::app().setThreadNum(4);
  drogon::app().addListener("0.0.0.0", static_cast<uint16_t>(config.port));
  drogon::app().run();
  return 0;
}
