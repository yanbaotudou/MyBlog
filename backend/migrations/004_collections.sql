CREATE TABLE IF NOT EXISTS collections (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  description TEXT NOT NULL DEFAULT '',
  owner_id INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  is_deleted INTEGER NOT NULL DEFAULT 0,
  FOREIGN KEY (owner_id) REFERENCES users(id),
  UNIQUE(owner_id, name)
);

CREATE INDEX IF NOT EXISTS idx_collections_owner ON collections(owner_id, updated_at DESC);

CREATE TABLE IF NOT EXISTS collection_posts (
  collection_id INTEGER NOT NULL,
  post_id INTEGER NOT NULL,
  position INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  PRIMARY KEY (collection_id, post_id),
  UNIQUE(collection_id, position),
  FOREIGN KEY (collection_id) REFERENCES collections(id),
  FOREIGN KEY (post_id) REFERENCES posts(id)
);

CREATE INDEX IF NOT EXISTS idx_collection_posts_post ON collection_posts(post_id);
