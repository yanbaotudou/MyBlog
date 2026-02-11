cmake_minimum_required(VERSION 3.16)
project(study_blog_api LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)
find_path(SQLITE3_INCLUDE_DIR sqlite3.h REQUIRED)
find_library(ARGON2_LIBRARY argon2 REQUIRED)
find_path(ARGON2_INCLUDE_DIR argon2.h REQUIRED)

add_executable(blog_api
  src/main.cc
  src/app/AppConfig.cc
  src/db/Database.cc
  src/db/Migrations.cc
  src/auth/JwtService.cc
  src/auth/PasswordService.cc
  src/auth/RefreshTokenService.cc
  src/middleware/AuthMiddleware.cc
  src/middleware/AdminMiddleware.cc
  src/controllers/AuthController.cc
  src/controllers/PostController.cc
  src/controllers/SearchController.cc
  src/controllers/AdminController.cc
  src/controllers/CollectionController.cc
  src/controllers/InteractionController.cc
  src/repositories/UserRepository.cc
  src/repositories/PostRepository.cc
  src/repositories/SearchRepository.cc
  src/repositories/CollectionRepository.cc
  src/repositories/InteractionRepository.cc
  src/utils/ApiError.cc
  src/utils/Validation.cc
)

target_include_directories(blog_api PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SQLITE3_INCLUDE_DIR}
  ${ARGON2_INCLUDE_DIR}
)

target_link_libraries(blog_api PRIVATE
  Drogon::Drogon
  ${SQLITE3_LIBRARY}
  ${ARGON2_LIBRARY}
  OpenSSL::SSL
  OpenSSL::Crypto
)
