# 学习日志博客网站（前后端分离 | C++ Drogon + React + SQLite）

这是一个可运行的前后端分离学习日志博客项目，满足以下闭环：

- 未登录可浏览文章列表、文章详情、全文搜索
- 登录后可发帖、编辑自己的文章、删除自己的文章
- 登录后可修改自己的密码
- 登录后可通过头像进入个人中心，管理自己的文章与合集
- 登录后可创建合集并将文章按顺序加入合集，文章页支持合集内上一篇/下一篇跳转
- 登录后可对文章点赞、收藏与评论（支持删除自己的评论，管理员可删任意评论）
- 管理员可查看用户、改角色、封禁/解封用户
- 后端 JWT 鉴权（Access Token）+ HttpOnly Refresh Token
- SQLite + FTS5 全文搜索

## 为什么后端选择 C++

原因：

1. 满足“优先后端使用 C++”的要求
2. Drogon 对 REST API、JSON、路由、中间件式处理支持成熟
3. 配合 SQLite（sqlite3）、Argon2、OpenSSL 可以完整实现鉴权、安全与工程化要求

## 技术栈

- 前端：React + Vite + TypeScript + React Router + React Markdown
- 后端：C++17 + Drogon + SQLite3 + Argon2id + OpenSSL
- 搜索：SQLite FTS5（`posts_fts` + 触发器自动同步）
- 容器：Docker + docker-compose

## 项目结构

```text
MyBlog/
├── docker-compose.yml
├── .env.example
├── README.md
├── backend/
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── config/
│   │   └── config.example.json
│   ├── migrations/
│   │   ├── 001_init.sql
│   │   ├── 002_fts.sql
│   │   ├── 003_timestamp_normalize.sql
│   │   ├── 004_collections.sql
│   │   └── 005_interactions.sql
│   ├── scripts/
│   │   ├── migrate.sh
│   │   └── seed_admin.sh
│   ├── src/
│   │   ├── main.cc
│   │   ├── app/
│   │   │   ├── AppConfig.h
│   │   │   └── AppConfig.cc
│   │   ├── db/
│   │   │   ├── Database.h
│   │   │   ├── Database.cc
│   │   │   └── Migrations.cc
│   │   ├── models/
│   │   │   ├── User.h
│   │   │   ├── Post.h
│   │   │   ├── Collection.h
│   │   │   ├── Comment.h
│   │   │   └── Interaction.h
│   │   ├── auth/
│   │   │   ├── JwtService.h
│   │   │   ├── JwtService.cc
│   │   │   ├── PasswordService.h
│   │   │   ├── PasswordService.cc
│   │   │   ├── RefreshTokenService.h
│   │   │   └── RefreshTokenService.cc
│   │   ├── middleware/
│   │   │   ├── AuthMiddleware.h
│   │   │   ├── AuthMiddleware.cc
│   │   │   ├── AdminMiddleware.h
│   │   │   └── AdminMiddleware.cc
│   │   ├── controllers/
│   │   │   ├── AuthController.h
│   │   │   ├── AuthController.cc
│   │   │   ├── PostController.h
│   │   │   ├── PostController.cc
│   │   │   ├── SearchController.h
│   │   │   ├── SearchController.cc
│   │   │   ├── AdminController.h
│   │   │   ├── AdminController.cc
│   │   │   ├── CollectionController.h
│   │   │   ├── CollectionController.cc
│   │   │   ├── InteractionController.h
│   │   │   └── InteractionController.cc
│   │   ├── repositories/
│   │   │   ├── UserRepository.h
│   │   │   ├── UserRepository.cc
│   │   │   ├── PostRepository.h
│   │   │   ├── PostRepository.cc
│   │   │   ├── SearchRepository.h
│   │   │   ├── SearchRepository.cc
│   │   │   ├── CollectionRepository.h
│   │   │   ├── CollectionRepository.cc
│   │   │   ├── InteractionRepository.h
│   │   │   └── InteractionRepository.cc
│   │   ├── utils/
│   │   │   ├── ApiError.h
│   │   │   ├── ApiError.cc
│   │   │   ├── Validation.h
│   │   │   ├── Validation.cc
│   │   │   └── JsonResponse.h
│   │   └── logging/
│   │       └── RequestLogger.h
│   └── tests/
│       └── api_smoke.sh
└── frontend/
    ├── Dockerfile
    ├── nginx.conf
    ├── package.json
    ├── tsconfig.json
    ├── vite.config.ts
    ├── index.html
    └── src/
        ├── main.tsx
        ├── App.tsx
        ├── api/
        │   ├── client.ts
        │   ├── auth.ts
        │   ├── posts.ts
        │   ├── search.ts
        │   ├── admin.ts
        │   ├── collections.ts
        │   └── interactions.ts
        ├── types/
        │   ├── api.ts
        │   ├── auth.ts
        │   ├── post.ts
        │   ├── user.ts
        │   ├── collection.ts
        │   └── interaction.ts
        ├── store/
        │   └── authStore.ts
        ├── components/
        │   ├── Header.tsx
        │   ├── Pagination.tsx
        │   ├── PostCard.tsx
        │   ├── ProtectedRoute.tsx
        │   ├── AdminRoute.tsx
        │   └── MarkdownEditor.tsx
        ├── pages/
        │   ├── HomePage.tsx
        │   ├── PostDetailPage.tsx
        │   ├── LoginPage.tsx
        │   ├── ChangePasswordPage.tsx
        │   ├── EditorPage.tsx
        │   ├── SearchPage.tsx
        │   ├── AdminUsersPage.tsx
        │   ├── CollectionsPage.tsx
        │   └── CollectionDetailPage.tsx
        ├── styles/
        │   └── index.css
        └── utils/
            └── dateTime.ts
```

## 先决条件

### Docker 方案（推荐）

- Docker >= 24
- Docker Compose Plugin

### 本地开发方案

- C++17 toolchain
- CMake >= 3.16
- Drogon（开发包）
- sqlite3 + 开发头文件
- argon2 + 开发头文件
- OpenSSL + 开发头文件
- Node.js >= 18
- npm >= 9

## 一键启动（docker-compose）

1. 复制环境变量：

```bash
cp .env.example .env
```

2. 修改 `.env`（至少改 JWT_SECRET、管理员密码、站点 URL）：

- `JWT_SECRET`
- `ADMIN_SEED_PASSWORD`
- `VITE_SITE_URL`（生产环境填你的公网域名，如 `https://blog.example.com`）
- 可选镜像覆盖（网络受限时）：
  `UBUNTU_IMAGE`、`NODE_IMAGE`、`NGINX_IMAGE`

3. 启动：

```bash
docker compose up --build
```

4. 访问：

- 前端：`http://localhost:5173`
- 后端 API：`http://localhost:8080`

## 阿里云 ECS 部署（Ubuntu）

适用场景：把当前项目部署到阿里云服务器并长期运行。

### 1) 服务器与安全组准备

1. 系统建议：Ubuntu 22.04 / 24.04。
2. 安全组放行：
- `22`（SSH）
- `80`（HTTP，建议生产使用）
- `443`（HTTPS，建议生产使用）
- `5173`（如果你按当前 compose 直接对外暴露前端）
3. 不建议暴露 `8080` 到公网（后端端口）。

### 2) 安装 Docker（稳定方案，推荐）

若你所在网络到 `download.docker.com` 不稳定，优先使用 Ubuntu 仓库：

```bash
sudo rm -f /etc/apt/sources.list.d/docker.list \
  /etc/apt/sources.list.d/docker-ce.list \
  /etc/apt/sources.list.d/docker-ce.sources \
  /etc/apt/keyrings/docker.gpg

sudo apt update
sudo apt install -y docker.io docker-compose-v2
sudo systemctl enable --now docker

docker --version
docker compose version
sudo docker run --rm hello-world
```

### 3) 拉取项目并配置环境

```bash
cd ~
git clone <你的仓库地址> MyBlog
cd MyBlog

cp .env.example .env
```

编辑 `.env`，至少修改：

- `APP_ENV=production`
- `JWT_SECRET=<长随机字符串>`
- `ADMIN_SEED_PASSWORD=<强密码>`
- `VITE_SITE_URL=http://<你的公网IP>:5173`（有域名就改为域名）
- `CORS_ALLOW_ORIGIN=http://<你的公网IP>:5173`

如果拉镜像超时（如 Docker Hub 不稳定），同时设置：

- `UBUNTU_IMAGE=registry.cn-hangzhou.aliyuncs.com/library/ubuntu:24.04`
- `NODE_IMAGE=registry.cn-hangzhou.aliyuncs.com/library/node:20-alpine`
- `NGINX_IMAGE=registry.cn-hangzhou.aliyuncs.com/library/nginx:1.27-alpine`

生成随机密钥示例：

```bash
openssl rand -hex 32
```

### 4) 启动服务

```bash
export COMPOSE_BAKE=false
docker compose up -d --build
docker compose ps
docker compose logs -f backend
docker compose logs -f frontend
```

访问：

- 前端：`http://<你的公网IP>:5173`
- 后端：`http://<你的公网IP>:8080`

### 5) 首次登录与初始化

1. 使用管理员账号登录：
- 用户名：`ADMIN_SEED_USERNAME`（默认 `admin`）
- 密码：`ADMIN_SEED_PASSWORD`
2. 首次登录后立刻修改管理员密码。

### 6) 日常更新

```bash
cd ~/MyBlog
git pull
docker compose up -d --build
```

### 7) 数据备份（SQLite）

数据库在宿主机 `./data/blog.db`（项目根目录下）：

```bash
cd ~/MyBlog
cp data/blog.db data/blog_$(date +%F_%H%M%S).db
```

## 本地启动（前后端分开）

### 后端

```bash
cp .env.example .env
export $(grep -v '^#' .env | xargs)

cmake -S backend -B backend/build
cmake --build backend/build
./backend/build/blog_api
```

若你之前从旧配置复制过 `.env`，请确认：

```bash
DB_PATH=./data/blog.db
MIGRATIONS_DIR=./backend/migrations
```

### 前端

```bash
cd frontend
npm install
npm run dev
```

浏览器打开 `http://localhost:5173`。

## 初始化数据库 / 迁移 / 种子管理员

### 自动方式（推荐）

后端启动时自动执行：

1. 建立数据库目录（按 `DB_PATH`）
2. 运行 `migrations/*.sql`
3. 自动创建默认管理员（仅首次）

默认账号来源：

- `ADMIN_SEED_USERNAME`（默认 `admin`）
- `ADMIN_SEED_PASSWORD`（默认 `ChangeMe123!`）

> 首次启动后请立刻修改管理员密码。

### 手动迁移（可选）

```bash
bash backend/scripts/migrate.sh
```

## API 总览

### 鉴权

- `POST /api/auth/register`
- `POST /api/auth/login`
- `POST /api/auth/refresh`
- `POST /api/auth/logout`
- `POST /api/auth/change-password`

### 文章

- `GET /api/posts?page=&pageSize=`
- `GET /api/me/posts?page=&pageSize=` (登录后查看我的文章)
- `GET /api/posts/:id`
- `POST /api/posts`
- `PUT /api/posts/:id`
- `DELETE /api/posts/:id`

### 互动

- `GET /api/posts/:id/interactions`
- `PUT /api/posts/:id/like`
- `DELETE /api/posts/:id/like`
- `PUT /api/posts/:id/favorite`
- `DELETE /api/posts/:id/favorite`
- `GET /api/me/favorites?page=&pageSize=&q=&order=desc|asc`
- `GET /api/posts/:id/comments?page=&pageSize=`
- `POST /api/posts/:id/comments`
- `DELETE /api/comments/:id`

### 搜索

- `GET /api/search?q=&page=&pageSize=`

### 管理员

- `GET /api/admin/users`
- `PUT /api/admin/users/:id/role`
- `PUT /api/admin/users/:id/ban`

### 合集

- `POST /api/collections`
- `GET /api/collections/mine`
- `GET /api/collections/:id`
- `POST /api/collections/:id/posts`
- `DELETE /api/collections/:id/posts/:postId`
- `GET /api/posts/:id/collections?collectionId=`

## 合集公开页 SEO / 分享

`/collections/:id` 已实现以下能力：

1. 动态 SEO 头信息：
- `title`、`description`、`keywords`
- `canonical`
- `Open Graph`（`og:title/description/url/image`）
- `Twitter Card`
2. 结构化数据：
- 注入 `CollectionPage + ItemList` JSON-LD
3. 分享能力：
- 复制合集链接
- 复制合集目录（含文章链接）
- 系统原生分享（支持 `navigator.share` 的浏览器）
- 微博 / X 一键分享
4. 默认分享封面：
- `frontend/public/og-cover.svg`

说明：前端通过 `VITE_SITE_URL` 生成 canonical 与分享绝对链接；未配置时自动回退到当前页面 origin。

## 统一响应格式

### 成功

```json
{
  "code": "OK",
  "message": "success",
  "data": {},
  "requestId": "uuid"
}
```

### 失败

```json
{
  "code": "AUTH_INVALID_TOKEN",
  "message": "token expired",
  "details": {},
  "requestId": "uuid"
}
```

## curl 示例（覆盖核心流程）

假设：

```bash
BASE="http://localhost:8080"
```

### 1) 注册

```bash
curl -i -c cookies.txt -X POST "$BASE/api/auth/register" \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"AlicePass123!"}'
```

### 2) 登录

```bash
curl -s -c cookies.txt -X POST "$BASE/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"AlicePass123!"}'
```

从返回 JSON 取 `data.accessToken`：

```bash
TOKEN="<粘贴accessToken>"
```

### 3) 刷新 token

```bash
curl -i -b cookies.txt -c cookies.txt -X POST "$BASE/api/auth/refresh"
```

### 4) 修改密码

```bash
curl -s -b cookies.txt -c cookies.txt -X POST "$BASE/api/auth/change-password" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"currentPassword":"AlicePass123!","newPassword":"AlicePass456!"}'
```

修改成功后，使用新返回的 `data.accessToken` 覆盖 `TOKEN`，后续请求继续使用新 token。

### 5) 发帖

```bash
curl -s -X POST "$BASE/api/posts" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"title":"我的第一篇学习日志","contentMarkdown":"今天学了 JWT 与 FTS5。"}'
```

### 6) 改帖

```bash
curl -s -X PUT "$BASE/api/posts/1" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"title":"更新后的标题","contentMarkdown":"补充：也实现了 refresh token 轮换。"}'
```

### 7) 再发一篇文章（用于合集示例）

```bash
curl -s -X POST "$BASE/api/posts" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"title":"第二篇学习日志","contentMarkdown":"用于合集导航示例。"}'
```

假设你拿到了两篇文章 ID：

```bash
POST1_ID=1
POST2_ID=2
```

### 8) 搜索（FTS5）

```bash
curl -s "$BASE/api/search?q=JWT&page=1&pageSize=10"
```

### 8.1) 点赞 / 收藏 / 评论

```bash
# 点赞
curl -s -X PUT "$BASE/api/posts/$POST1_ID/like" \
  -H "Authorization: Bearer $TOKEN"
```

```bash
# 收藏
curl -s -X PUT "$BASE/api/posts/$POST1_ID/favorite" \
  -H "Authorization: Bearer $TOKEN"
```

```bash
# 查看互动统计（未登录也可）
curl -s "$BASE/api/posts/$POST1_ID/interactions"
```

```bash
# 查看我的收藏文章（登录后）
curl -s "$BASE/api/me/favorites?page=1&pageSize=10" \
  -H "Authorization: Bearer $TOKEN"
```

```bash
# 收藏搜索 + 排序（收藏时间）
curl -s "$BASE/api/me/favorites?page=1&pageSize=10&q=JWT&order=desc" \
  -H "Authorization: Bearer $TOKEN"
```

```bash
# 发表评论
curl -s -X POST "$BASE/api/posts/$POST1_ID/comments" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"content":"写得很好，收藏了。"}'
```

```bash
# 查看评论列表
curl -s "$BASE/api/posts/$POST1_ID/comments?page=1&pageSize=10"
```

```bash
# 删除评论（评论作者或管理员）
curl -s -X DELETE "$BASE/api/comments/1" \
  -H "Authorization: Bearer $TOKEN"
```

```bash
# 取消点赞 / 取消收藏
curl -s -X DELETE "$BASE/api/posts/$POST1_ID/like" -H "Authorization: Bearer $TOKEN"
curl -s -X DELETE "$BASE/api/posts/$POST1_ID/favorite" -H "Authorization: Bearer $TOKEN"
```

### 9) 创建合集

```bash
curl -s -X POST "$BASE/api/collections" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"name":"JWT 学习合集","description":"按顺序整理 JWT 学习文章"}'
```

假设返回了 `COLLECTION_ID=1`：

```bash
COLLECTION_ID=1
```

### 10) 将文章加入合集（按加入顺序）

```bash
curl -s -X POST "$BASE/api/collections/$COLLECTION_ID/posts" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"postId\":$POST1_ID}"
```

```bash
curl -s -X POST "$BASE/api/collections/$COLLECTION_ID/posts" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"postId\":$POST2_ID}"
```

### 11) 查询文章所在合集 + 上一篇/下一篇

```bash
curl -s "$BASE/api/posts/$POST1_ID/collections?collectionId=$COLLECTION_ID"
```

### 12) 从合集移除文章

```bash
curl -s -X DELETE "$BASE/api/collections/$COLLECTION_ID/posts/$POST1_ID" \
  -H "Authorization: Bearer $TOKEN"
```

### 13) 管理员登录

```bash
curl -s -c admin_cookies.txt -X POST "$BASE/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"ChangeMe123!"}'
```

```bash
ADMIN_TOKEN="<粘贴管理员accessToken>"
```

### 14) 管理员查看用户

```bash
curl -s "$BASE/api/admin/users?page=1&pageSize=20" \
  -H "Authorization: Bearer $ADMIN_TOKEN"
```

### 15) 管理员改角色

```bash
curl -s -X PUT "$BASE/api/admin/users/2/role" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"role":"admin"}'
```

### 16) 管理员封禁/解封

```bash
curl -s -X PUT "$BASE/api/admin/users/2/ban" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"isBanned":true}'
```

```bash
curl -s -X PUT "$BASE/api/admin/users/2/ban" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"isBanned":false}'
```

### 17) 删帖（软删除）

```bash
curl -s -X DELETE "$BASE/api/posts/$POST1_ID" \
  -H "Authorization: Bearer $TOKEN"
```

### 18) 退出登录

```bash
curl -i -b cookies.txt -X POST "$BASE/api/auth/logout"
```

## 测试

### 后端 smoke 测试

后端启动后执行：

```bash
bash backend/tests/api_smoke.sh
```

可选环境变量：

- `BASE_URL`
- `ADMIN_USER`
- `ADMIN_PASS`

`api_smoke.sh` 已覆盖注册、登录、刷新、修改密码、发帖、改帖、搜索、点赞/收藏/评论、合集创建与导航、权限校验、管理员操作与删帖。

## 安全说明

1. 密码哈希：Argon2id（非明文）
2. Access Token：JWT（短期）
3. Refresh Token：HttpOnly Cookie + 服务端哈希存储 + 轮换
4. CORS：可配置来源，允许 credentials
5. 输入校验：用户名/密码/标题/正文/分页/角色
6. 统一错误码：`AUTH_REQUIRED`、`AUTH_INVALID_TOKEN`、`FORBIDDEN`、`USER_BANNED` 等

## 前端登录持久化策略与权衡

当前实现：

- Access Token 存储在 `localStorage`
- Refresh Token 存储在 `HttpOnly Cookie`

权衡：

1. 优点：刷新页面后可保持登录态，体验好
2. 风险：若站点发生 XSS，`localStorage` 中 access token 有泄露风险
3. 建议：生产环境可改为内存 token + 更严格 CSP + 更强输入输出净化

## 常见问题排查

### 1) 端口占用

- 后端默认 `8080`
- 前端默认 `5173`

修改 `.env` 与 `docker-compose.yml` 端口映射后重启。

### 2) CORS 报错

检查：

- `CORS_ALLOW_ORIGIN` 是否与前端实际地址一致
- 请求是否带 `credentials: include`

### 3) JWT 失效 / 频繁 401

检查：

- `JWT_SECRET` 是否被修改导致旧 token 失效
- access token 是否过期（默认 15 分钟）
- 是否调用了 `/api/auth/refresh`

### 4) refresh 不生效

检查：

- 浏览器是否接受 Cookie
- 请求是否包含 `credentials`
- 后端是否返回 `Set-Cookie` 头

### 5) SQLite 文件位置

数据库路径由 `DB_PATH` 决定：

- Docker 默认：`/app/data/blog.db`（映射到宿主机 `./data/blog.db`）
- 本地默认：`./data/blog.db`

### 6) 默认管理员无法登录

检查：

- 首次启动前是否设置了 `ADMIN_SEED_PASSWORD`
- 用户名是否已存在（已存在则不会覆盖密码）

## 生产部署建议

1. 必改 `JWT_SECRET` 与管理员初始密码
2. 启用 HTTPS（配合 `Secure` Cookie）
3. 使用反向代理统一入口并限制来源
4. 开启日志集中化与审计
5. 定期备份 SQLite（或迁移到独立数据库）
