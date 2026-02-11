# Details

Date : 2026-02-10 17:22:34

Directory /home/xiaohu/workspace/MyBlog

Total : 79 files,  4762 codes, 3 comments, 1072 blanks, all 5837 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [README.md](/README.md) | Markdown | 364 | 0 | 120 | 484 |
| [backend/CMakeLists.txt](/backend/CMakeLists.txt) | CMake | 43 | 0 | 6 | 49 |
| [backend/Dockerfile](/backend/Dockerfile) | Docker | 10 | 0 | 4 | 14 |
| [backend/config/config.example.json](/backend/config/config.example.json) | JSON | 16 | 0 | 1 | 17 |
| [backend/migrations/001\_init.sql](/backend/migrations/001_init.sql) | MS SQL | 36 | 0 | 6 | 42 |
| [backend/migrations/002\_fts.sql](/backend/migrations/002_fts.sql) | MS SQL | 31 | 0 | 6 | 37 |
| [backend/scripts/migrate.sh](/backend/scripts/migrate.sh) | Shell Script | 19 | 1 | 7 | 27 |
| [backend/scripts/seed\_admin.sh](/backend/scripts/seed_admin.sh) | Shell Script | 4 | 1 | 2 | 7 |
| [backend/src/app/AppConfig.cc](/backend/src/app/AppConfig.cc) | C++ | 45 | 0 | 10 | 55 |
| [backend/src/app/AppConfig.h](/backend/src/app/AppConfig.h) | C++ | 20 | 0 | 7 | 27 |
| [backend/src/auth/JwtService.cc](/backend/src/auth/JwtService.cc) | C++ | 186 | 0 | 40 | 226 |
| [backend/src/auth/JwtService.h](/backend/src/auth/JwtService.h) | C++ | 23 | 0 | 9 | 32 |
| [backend/src/auth/PasswordService.cc](/backend/src/auth/PasswordService.cc) | C++ | 41 | 0 | 11 | 52 |
| [backend/src/auth/PasswordService.h](/backend/src/auth/PasswordService.h) | C++ | 9 | 0 | 5 | 14 |
| [backend/src/auth/RefreshTokenService.cc](/backend/src/auth/RefreshTokenService.cc) | C++ | 256 | 0 | 51 | 307 |
| [backend/src/auth/RefreshTokenService.h](/backend/src/auth/RefreshTokenService.h) | C++ | 27 | 0 | 11 | 38 |
| [backend/src/controllers/AdminController.cc](/backend/src/controllers/AdminController.cc) | C++ | 146 | 0 | 31 | 177 |
| [backend/src/controllers/AdminController.h](/backend/src/controllers/AdminController.h) | C++ | 22 | 0 | 11 | 33 |
| [backend/src/controllers/AuthController.cc](/backend/src/controllers/AuthController.cc) | C++ | 166 | 0 | 41 | 207 |
| [backend/src/controllers/AuthController.h](/backend/src/controllers/AuthController.h) | C++ | 29 | 0 | 12 | 41 |
| [backend/src/controllers/PostController.cc](/backend/src/controllers/PostController.cc) | C++ | 177 | 0 | 41 | 218 |
| [backend/src/controllers/PostController.h](/backend/src/controllers/PostController.h) | C++ | 31 | 0 | 13 | 44 |
| [backend/src/controllers/SearchController.cc](/backend/src/controllers/SearchController.cc) | C++ | 52 | 0 | 14 | 66 |
| [backend/src/controllers/SearchController.h](/backend/src/controllers/SearchController.h) | C++ | 14 | 0 | 9 | 23 |
| [backend/src/db/Database.cc](/backend/src/db/Database.cc) | C++ | 56 | 0 | 13 | 69 |
| [backend/src/db/Database.h](/backend/src/db/Database.h) | C++ | 20 | 0 | 10 | 30 |
| [backend/src/db/Migrations.cc](/backend/src/db/Migrations.cc) | C++ | 127 | 0 | 29 | 156 |
| [backend/src/logging/RequestLogger.h](/backend/src/logging/RequestLogger.h) | C++ | 19 | 0 | 6 | 25 |
| [backend/src/main.cc](/backend/src/main.cc) | C++ | 209 | 0 | 39 | 248 |
| [backend/src/middleware/AdminMiddleware.cc](/backend/src/middleware/AdminMiddleware.cc) | C++ | 10 | 0 | 4 | 14 |
| [backend/src/middleware/AdminMiddleware.h](/backend/src/middleware/AdminMiddleware.h) | C++ | 9 | 0 | 5 | 14 |
| [backend/src/middleware/AuthMiddleware.cc](/backend/src/middleware/AuthMiddleware.cc) | C++ | 50 | 0 | 11 | 61 |
| [backend/src/middleware/AuthMiddleware.h](/backend/src/middleware/AuthMiddleware.h) | C++ | 22 | 0 | 8 | 30 |
| [backend/src/models/Post.h](/backend/src/models/Post.h) | C++ | 15 | 0 | 5 | 20 |
| [backend/src/models/User.h](/backend/src/models/User.h) | C++ | 13 | 0 | 5 | 18 |
| [backend/src/repositories/PostRepository.cc](/backend/src/repositories/PostRepository.cc) | C++ | 218 | 0 | 49 | 267 |
| [backend/src/repositories/PostRepository.h](/backend/src/repositories/PostRepository.h) | C++ | 28 | 0 | 11 | 39 |
| [backend/src/repositories/SearchRepository.cc](/backend/src/repositories/SearchRepository.cc) | C++ | 128 | 0 | 25 | 153 |
| [backend/src/repositories/SearchRepository.h](/backend/src/repositories/SearchRepository.h) | C++ | 20 | 0 | 8 | 28 |
| [backend/src/repositories/UserRepository.cc](/backend/src/repositories/UserRepository.cc) | C++ | 309 | 0 | 63 | 372 |
| [backend/src/repositories/UserRepository.h](/backend/src/repositories/UserRepository.h) | C++ | 31 | 0 | 11 | 42 |
| [backend/src/utils/ApiError.cc](/backend/src/utils/ApiError.cc) | C++ | 11 | 0 | 4 | 15 |
| [backend/src/utils/ApiError.h](/backend/src/utils/ApiError.h) | C++ | 15 | 0 | 7 | 22 |
| [backend/src/utils/JsonResponse.h](/backend/src/utils/JsonResponse.h) | C++ | 37 | 0 | 11 | 48 |
| [backend/src/utils/Validation.cc](/backend/src/utils/Validation.cc) | C++ | 105 | 0 | 17 | 122 |
| [backend/src/utils/Validation.h](/backend/src/utils/Validation.h) | C++ | 24 | 0 | 8 | 32 |
| [backend/tests/api\_smoke.sh](/backend/tests/api_smoke.sh) | Shell Script | 80 | 1 | 22 | 103 |
| [docker-compose.yml](/docker-compose.yml) | YAML | 33 | 0 | 2 | 35 |
| [frontend/Dockerfile](/frontend/Dockerfile) | Docker | 11 | 0 | 2 | 13 |
| [frontend/index.html](/frontend/index.html) | HTML | 16 | 0 | 1 | 17 |
| [frontend/nginx.conf](/frontend/nginx.conf) | Properties | 16 | 0 | 4 | 20 |
| [frontend/package.json](/frontend/package.json) | JSON | 24 | 0 | 1 | 25 |
| [frontend/src/App.tsx](/frontend/src/App.tsx) | TypeScript JSX | 54 | 0 | 8 | 62 |
| [frontend/src/api/admin.ts](/frontend/src/api/admin.ts) | TypeScript | 25 | 0 | 5 | 30 |
| [frontend/src/api/auth.ts](/frontend/src/api/auth.ts) | TypeScript | 32 | 0 | 6 | 38 |
| [frontend/src/api/client.ts](/frontend/src/api/client.ts) | TypeScript | 55 | 0 | 12 | 67 |
| [frontend/src/api/posts.ts](/frontend/src/api/posts.ts) | TypeScript | 35 | 0 | 7 | 42 |
| [frontend/src/api/search.ts](/frontend/src/api/search.ts) | TypeScript | 12 | 0 | 2 | 14 |
| [frontend/src/components/AdminRoute.tsx](/frontend/src/components/AdminRoute.tsx) | TypeScript JSX | 17 | 0 | 5 | 22 |
| [frontend/src/components/Header.tsx](/frontend/src/components/Header.tsx) | TypeScript JSX | 64 | 0 | 8 | 72 |
| [frontend/src/components/MarkdownEditor.tsx](/frontend/src/components/MarkdownEditor.tsx) | TypeScript JSX | 37 | 0 | 5 | 42 |
| [frontend/src/components/Pagination.tsx](/frontend/src/components/Pagination.tsx) | TypeScript JSX | 22 | 0 | 3 | 25 |
| [frontend/src/components/PostCard.tsx](/frontend/src/components/PostCard.tsx) | TypeScript JSX | 28 | 0 | 2 | 30 |
| [frontend/src/components/ProtectedRoute.tsx](/frontend/src/components/ProtectedRoute.tsx) | TypeScript JSX | 10 | 0 | 4 | 14 |
| [frontend/src/main.tsx](/frontend/src/main.tsx) | TypeScript JSX | 9 | 0 | 2 | 11 |
| [frontend/src/pages/AdminUsersPage.tsx](/frontend/src/pages/AdminUsersPage.tsx) | TypeScript JSX | 86 | 0 | 10 | 96 |
| [frontend/src/pages/EditorPage.tsx](/frontend/src/pages/EditorPage.tsx) | TypeScript JSX | 95 | 0 | 15 | 110 |
| [frontend/src/pages/HomePage.tsx](/frontend/src/pages/HomePage.tsx) | TypeScript JSX | 63 | 0 | 8 | 71 |
| [frontend/src/pages/LoginPage.tsx](/frontend/src/pages/LoginPage.tsx) | TypeScript JSX | 77 | 0 | 13 | 90 |
| [frontend/src/pages/PostDetailPage.tsx](/frontend/src/pages/PostDetailPage.tsx) | TypeScript JSX | 90 | 0 | 12 | 102 |
| [frontend/src/pages/SearchPage.tsx](/frontend/src/pages/SearchPage.tsx) | TypeScript JSX | 81 | 0 | 16 | 97 |
| [frontend/src/store/authStore.ts](/frontend/src/store/authStore.ts) | TypeScript | 59 | 0 | 14 | 73 |
| [frontend/src/styles/index.css](/frontend/src/styles/index.css) | PostCSS | 311 | 0 | 54 | 365 |
| [frontend/src/types/api.ts](/frontend/src/types/api.ts) | TypeScript | 12 | 0 | 2 | 14 |
| [frontend/src/types/auth.ts](/frontend/src/types/auth.ts) | TypeScript | 9 | 0 | 3 | 12 |
| [frontend/src/types/post.ts](/frontend/src/types/post.ts) | TypeScript | 16 | 0 | 2 | 18 |
| [frontend/src/types/user.ts](/frontend/src/types/user.ts) | TypeScript | 8 | 0 | 2 | 10 |
| [frontend/tsconfig.json](/frontend/tsconfig.json) | JSON with Comments | 16 | 0 | 1 | 17 |
| [frontend/vite.config.ts](/frontend/vite.config.ts) | TypeScript | 16 | 0 | 2 | 18 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)