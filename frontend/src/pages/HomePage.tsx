import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { listPosts } from "../api/posts";
import { Pagination } from "../components/Pagination";
import { PostCard } from "../components/PostCard";
import { useAuthState } from "../store/authStore";
import type { Post } from "../types/post";

export function HomePage() {
  const auth = useAuthState();
  const [items, setItems] = useState<Post[]>([]);
  const [page, setPage] = useState(1);
  const [pageSize] = useState(10);
  const [total, setTotal] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  useEffect(() => {
    const run = async () => {
      setLoading(true);
      setError("");
      try {
        const data = await listPosts(page, pageSize);
        setItems(data.items);
        setTotal(data.total);
      } catch (e) {
        setError(e instanceof Error ? e.message : "加载失败");
      } finally {
        setLoading(false);
      }
    };
    void run();
  }, [page, pageSize]);

  return (
    <main className="container">
      <section className="home-hero">
        <div className="hero-copy">
          <h1>学习日志博客</h1>
          <p>公开记录你的学习路径，持续迭代知识地图。</p>
          <div className="hero-stats">
            <span>公开文章：{total} 篇</span>
            <span>当前页：{page}</span>
          </div>
        </div>

        {auth.user ? (
          <Link className="primary-btn" to="/editor/new">
            新建文章
          </Link>
        ) : (
          <div className="login-gate">
            <button className="primary-btn" disabled>
              新建文章（需登录）
            </button>
            <Link to="/login">去登录</Link>
          </div>
        )}
      </section>

      {loading ? <p className="status-line">正在加载...</p> : null}
      {error ? <p className="status-line error">{error}</p> : null}

      {items.length === 0 && !loading ? <p className="empty-state">暂无文章，开始写第一篇吧。</p> : null}
      <section className="post-list">
        {items.map((post) => (
          <PostCard key={post.id} post={post} />
        ))}
      </section>

      <Pagination page={page} pageSize={pageSize} total={total} onPageChange={setPage} />
    </main>
  );
}
