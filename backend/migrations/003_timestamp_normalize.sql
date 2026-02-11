-- Normalize historical timestamps to UTC ISO-8601 with trailing Z.
-- Existing values were stored as "YYYY-MM-DD HH:MM:SS" (UTC by convention).

UPDATE users
SET created_at = strftime('%Y-%m-%dT%H:%M:%SZ', datetime(created_at))
WHERE created_at IS NOT NULL
  AND created_at NOT LIKE '%Z';

UPDATE posts
SET created_at = strftime('%Y-%m-%dT%H:%M:%SZ', datetime(created_at))
WHERE created_at IS NOT NULL
  AND created_at NOT LIKE '%Z';

UPDATE posts
SET updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', datetime(updated_at))
WHERE updated_at IS NOT NULL
  AND updated_at NOT LIKE '%Z';
