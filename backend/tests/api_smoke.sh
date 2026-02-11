#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:8080}"
ADMIN_USER="${ADMIN_USER:-admin}"
ADMIN_PASS="${ADMIN_PASS:-ChangeMe123!}"
USER_NAME="user_$RANDOM"
USER_PASS="UserPass123!"
USER_NEW_PASS="UserPass456!"
COOKIE_FILE="$(mktemp)"
ADMIN_COOKIE_FILE="$(mktemp)"

cleanup() {
  rm -f "$COOKIE_FILE" "$ADMIN_COOKIE_FILE"
}
trap cleanup EXIT

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required for this test script"
  exit 1
fi

echo "[1/17] Register user: $USER_NAME"
REGISTER_RESP=$(curl -sS -c "$COOKIE_FILE" -X POST "$BASE_URL/api/auth/register" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"$USER_NAME\",\"password\":\"$USER_PASS\"}")
USER_TOKEN=$(echo "$REGISTER_RESP" | jq -r '.data.accessToken')
USER_ID=$(echo "$REGISTER_RESP" | jq -r '.data.user.id')

[ "$USER_TOKEN" != "null" ]

echo "[2/17] Login user"
LOGIN_RESP=$(curl -sS -c "$COOKIE_FILE" -X POST "$BASE_URL/api/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"$USER_NAME\",\"password\":\"$USER_PASS\"}")
USER_TOKEN=$(echo "$LOGIN_RESP" | jq -r '.data.accessToken')

[ "$USER_TOKEN" != "null" ]

echo "[3/17] Create post #1"
CREATE_RESP=$(curl -sS -X POST "$BASE_URL/api/posts" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d '{"title":"FTS Demo","contentMarkdown":"今天学习了 SQLite FTS5 搜索。"}')
POST_ID=$(echo "$CREATE_RESP" | jq -r '.data.id')

[ "$POST_ID" != "null" ]

echo "[4/17] Create post #2"
CREATE_RESP_2=$(curl -sS -X POST "$BASE_URL/api/posts" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d '{"title":"Collections Demo","contentMarkdown":"合集导航：上一篇与下一篇。"}')
POST_ID_2=$(echo "$CREATE_RESP_2" | jq -r '.data.id')

[ "$POST_ID_2" != "null" ]

echo "[5/17] Update post #1"
curl -sS -X PUT "$BASE_URL/api/posts/$POST_ID" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d '{"title":"FTS Demo Updated","contentMarkdown":"更新：学习了 JWT + refresh token。"}' \
  | jq -e '.code == "OK"' >/dev/null

echo "[6/17] Create collection and add posts"
COLLECTION_RESP=$(curl -sS -X POST "$BASE_URL/api/collections" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d '{"name":"Smoke Collection","description":"用于合集功能冒烟测试"}')
COLLECTION_ID=$(echo "$COLLECTION_RESP" | jq -r '.data.id')

[ "$COLLECTION_ID" != "null" ]

curl -sS -X POST "$BASE_URL/api/collections/$COLLECTION_ID/posts" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d "{\"postId\":$POST_ID}" \
  | jq -e '.data.added == true' >/dev/null

curl -sS -X POST "$BASE_URL/api/collections/$COLLECTION_ID/posts" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d "{\"postId\":$POST_ID_2}" \
  | jq -e '.data.added == true' >/dev/null

echo "[7/17] Verify collection detail and prev/next navigation"
curl -sS "$BASE_URL/api/collections/$COLLECTION_ID" \
  | jq -e ".data.total == 2 and .data.posts[0].id == $POST_ID and .data.posts[1].id == $POST_ID_2" >/dev/null

curl -sS "$BASE_URL/api/posts/$POST_ID/collections?collectionId=$COLLECTION_ID" \
  | jq -e ".data.navigation.currentPosition == 1 and .data.navigation.next.id == $POST_ID_2" >/dev/null

echo "[8/17] Remove one post from collection and verify reorder"
curl -sS -X DELETE "$BASE_URL/api/collections/$COLLECTION_ID/posts/$POST_ID" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.removed == true' >/dev/null

curl -sS "$BASE_URL/api/collections/$COLLECTION_ID" \
  | jq -e ".data.total == 1 and .data.posts[0].id == $POST_ID_2 and .data.posts[0].collectionPosition == 1" >/dev/null

echo "[9/17] Search post"
curl -sS "$BASE_URL/api/search?q=JWT&page=1&pageSize=10" \
  | jq -e '.data.total >= 1' >/dev/null

echo "[10/17] Like/Favorite/Comment interactions"
curl -sS "$BASE_URL/api/posts/$POST_ID/interactions" \
  | jq -e '.data.likeCount == 0 and .data.favoriteCount == 0 and .data.commentCount == 0' >/dev/null

curl -sS -X PUT "$BASE_URL/api/posts/$POST_ID/like" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.likedByMe == true and .data.likeCount == 1' >/dev/null

curl -sS -X PUT "$BASE_URL/api/posts/$POST_ID/favorite" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.favoritedByMe == true and .data.favoriteCount == 1' >/dev/null

COMMENT_RESP=$(curl -sS -X POST "$BASE_URL/api/posts/$POST_ID/comments" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d '{"content":"这篇文章很有帮助"}')
COMMENT_ID=$(echo "$COMMENT_RESP" | jq -r '.data.id')
[ "$COMMENT_ID" != "null" ]

curl -sS "$BASE_URL/api/posts/$POST_ID/comments?page=1&pageSize=10" \
  | jq -e ".data.total >= 1 and .data.items[0].id == $COMMENT_ID" >/dev/null

curl -sS -X DELETE "$BASE_URL/api/comments/$COMMENT_ID" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.deleted == true' >/dev/null

curl -sS -X DELETE "$BASE_URL/api/posts/$POST_ID/like" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.likedByMe == false and .data.likeCount == 0' >/dev/null

curl -sS -X DELETE "$BASE_URL/api/posts/$POST_ID/favorite" \
  -H "Authorization: Bearer $USER_TOKEN" \
  | jq -e '.data.favoritedByMe == false and .data.favoriteCount == 0' >/dev/null

curl -sS "$BASE_URL/api/posts/$POST_ID/interactions" \
  | jq -e '.data.likeCount == 0 and .data.favoriteCount == 0 and .data.commentCount == 0' >/dev/null

echo "[11/17] Refresh token"
REFRESH_RESP=$(curl -sS -b "$COOKIE_FILE" -c "$COOKIE_FILE" -X POST "$BASE_URL/api/auth/refresh")
USER_TOKEN=$(echo "$REFRESH_RESP" | jq -r '.data.accessToken')

[ "$USER_TOKEN" != "null" ]

echo "[12/17] Change password and verify old password invalid"
CHANGE_RESP=$(curl -sS -b "$COOKIE_FILE" -c "$COOKIE_FILE" -X POST "$BASE_URL/api/auth/change-password" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $USER_TOKEN" \
  -d "{\"currentPassword\":\"$USER_PASS\",\"newPassword\":\"$USER_NEW_PASS\"}")
USER_TOKEN=$(echo "$CHANGE_RESP" | jq -r '.data.accessToken')
[ "$USER_TOKEN" != "null" ]

OLD_LOGIN_STATUS=$(curl -sS -o /tmp/old_password_login.json -w '%{http_code}' -X POST "$BASE_URL/api/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"$USER_NAME\",\"password\":\"$USER_PASS\"}")
[ "$OLD_LOGIN_STATUS" = "401" ]

echo "[13/17] Login with new password"
NEW_LOGIN_RESP=$(curl -sS -c "$COOKIE_FILE" -X POST "$BASE_URL/api/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"$USER_NAME\",\"password\":\"$USER_NEW_PASS\"}")
USER_TOKEN=$(echo "$NEW_LOGIN_RESP" | jq -r '.data.accessToken')
[ "$USER_TOKEN" != "null" ]

echo "[14/17] Login admin"
ADMIN_LOGIN=$(curl -sS -c "$ADMIN_COOKIE_FILE" -X POST "$BASE_URL/api/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"$ADMIN_USER\",\"password\":\"$ADMIN_PASS\"}")
ADMIN_TOKEN=$(echo "$ADMIN_LOGIN" | jq -r '.data.accessToken')

[ "$ADMIN_TOKEN" != "null" ]

echo "[15/17] Normal user tries admin API (expect 403)"
STATUS=$(curl -sS -o /tmp/admin_forbidden.json -w '%{http_code}' "$BASE_URL/api/admin/users" \
  -H "Authorization: Bearer $USER_TOKEN")
[ "$STATUS" = "403" ]

echo "[16/17] Admin updates user role and ban, then soft delete posts"
curl -sS -X PUT "$BASE_URL/api/admin/users/$USER_ID/role" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"role":"admin"}' \
  | jq -e '.data.role == "admin"' >/dev/null

curl -sS -X PUT "$BASE_URL/api/admin/users/$USER_ID/ban" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"isBanned":true}' \
  | jq -e '.data.isBanned == true' >/dev/null

curl -sS -X PUT "$BASE_URL/api/admin/users/$USER_ID/ban" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"isBanned":false}' \
  | jq -e '.data.isBanned == false' >/dev/null

curl -sS -X DELETE "$BASE_URL/api/posts/$POST_ID" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  | jq -e '.data.deleted == true' >/dev/null

curl -sS -X DELETE "$BASE_URL/api/posts/$POST_ID_2" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  | jq -e '.data.deleted == true' >/dev/null

echo "[17/17] Completed all checks"

echo "Smoke test completed successfully."
