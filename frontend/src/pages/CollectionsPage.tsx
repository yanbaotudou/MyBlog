import { FormEvent, useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { createCollection, listMyCollections } from "../api/collections";
import type { Collection } from "../types/collection";
import { formatAbsoluteDateTime } from "../utils/dateTime";

export function CollectionsPage() {
  const [collections, setCollections] = useState<Collection[]>([]);
  const [name, setName] = useState("");
  const [description, setDescription] = useState("");
  const [loading, setLoading] = useState(false);
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState("");

  const loadCollections = async () => {
    setLoading(true);
    setError("");
    try {
      const data = await listMyCollections();
      setCollections(data.items);
    } catch (e) {
      setError(e instanceof Error ? e.message : "加载合集失败");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    void loadCollections();
  }, []);

  const handleCreate = async (e: FormEvent) => {
    e.preventDefault();
    if (!name.trim()) {
      setError("合集名称不能为空");
      return;
    }

    setSubmitting(true);
    setError("");
    try {
      await createCollection({
        name: name.trim(),
        description: description.trim()
      });
      setName("");
      setDescription("");
      await loadCollections();
    } catch (e) {
      setError(e instanceof Error ? e.message : "创建合集失败");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="container collections-page">
      <section className="collection-create-card">
        <h1>我的合集</h1>
        <p>创建学习主题合集，把相关文章按顺序组织起来。</p>

        <form onSubmit={handleCreate} className="collection-form">
          <label htmlFor="collection-name">合集名称</label>
          <input
            id="collection-name"
            value={name}
            onChange={(e) => setName(e.target.value)}
            maxLength={80}
            placeholder="例如：从零实现博客系统"
          />

          <label htmlFor="collection-desc">合集简介</label>
          <textarea
            id="collection-desc"
            value={description}
            onChange={(e) => setDescription(e.target.value)}
            maxLength={500}
            rows={3}
            placeholder="简单描述这个合集的目标与范围"
          />

          <button type="submit" className="primary-btn" disabled={submitting}>
            {submitting ? "创建中..." : "创建合集"}
          </button>
        </form>

        {error ? <p className="status-line error">{error}</p> : null}
      </section>

      <section className="collection-list-wrap">
        <h2>合集列表</h2>
        {loading ? <p className="status-line">正在加载...</p> : null}

        {!loading && collections.length === 0 ? <p className="empty-state">你还没有创建合集。</p> : null}

        <div className="collection-grid">
          {collections.map((collection) => (
            <article key={collection.id} className="collection-card">
              <h3>{collection.name}</h3>
              <p>{collection.description || "暂无简介"}</p>
              <div className="post-meta">
                <span className="meta-pill">文章数：{collection.postCount}</span>
                <span className="meta-pill">更新：{formatAbsoluteDateTime(collection.updatedAt)}</span>
              </div>
              <Link className="secondary-btn" to={`/collections/${collection.id}`}>
                进入合集
              </Link>
            </article>
          ))}
        </div>
      </section>
    </main>
  );
}
