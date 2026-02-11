CREATE VIRTUAL TABLE IF NOT EXISTS posts_fts
USING fts5(
  title,
  content_markdown,
  content='posts',
  content_rowid='id'
);

INSERT INTO posts_fts(rowid, title, content_markdown)
SELECT id, title, content_markdown
FROM posts
WHERE is_deleted = 0
  AND id NOT IN (SELECT rowid FROM posts_fts);

CREATE TRIGGER IF NOT EXISTS posts_ai AFTER INSERT ON posts
WHEN new.is_deleted = 0
BEGIN
  INSERT INTO posts_fts(rowid, title, content_markdown)
  VALUES (new.id, new.title, new.content_markdown);
END;

CREATE TRIGGER IF NOT EXISTS posts_au AFTER UPDATE ON posts
BEGIN
  INSERT INTO posts_fts(posts_fts, rowid, title, content_markdown)
  VALUES ('delete', old.id, old.title, old.content_markdown);

  INSERT INTO posts_fts(rowid, title, content_markdown)
  SELECT new.id, new.title, new.content_markdown
  WHERE new.is_deleted = 0;
END;

CREATE TRIGGER IF NOT EXISTS posts_ad AFTER DELETE ON posts
BEGIN
  INSERT INTO posts_fts(posts_fts, rowid, title, content_markdown)
  VALUES ('delete', old.id, old.title, old.content_markdown);
END;
