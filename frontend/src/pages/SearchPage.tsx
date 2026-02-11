import { FormEvent, useEffect, useMemo, useState } from "react";
import { useNavigate, useSearchParams } from "react-router-dom";
import { searchPosts } from "../api/search";
import { Pagination } from "../components/Pagination";
import { PostCard } from "../components/PostCard";
import type { Post } from "../types/post";

export function SearchPage() {
  const navigate = useNavigate();
  const [params] = useSearchParams();

  const qFromUrl = params.get("q") || "";
  const pageFromUrl = Number(params.get("page") || "1") || 1;

  const [input, setInput] = useState(qFromUrl);
  const [items, setItems] = useState<Post[]>([]);
  const [page, setPage] = useState(pageFromUrl);
  const [total, setTotal] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  useEffect(() => {
    setInput(qFromUrl);
    setPage(pageFromUrl);
  }, [qFromUrl, pageFromUrl]);

  useEffect(() => {
    if (!qFromUrl) {
      setItems([]);
      setTotal(0);
      return;
    }

    const run = async () => {
      setLoading(true);
      setError("");
      try {
        const data = await searchPosts(qFromUrl, page, 10);
        setItems(data.items);
        setTotal(data.total);
      } catch (e) {
        setError(e instanceof Error ? e.message : "搜索失败");
      } finally {
        setLoading(false);
      }
    };

    void run();
  }, [qFromUrl, page]);

  const onSubmit = (e: FormEvent) => {
    e.preventDefault();
    const q = input.trim();
    if (!q) {
      return;
    }
    navigate(`/search?q=${encodeURIComponent(q)}&page=1`);
  };

  const onPageChange = (next: number) => {
    navigate(`/search?q=${encodeURIComponent(qFromUrl)}&page=${next}`);
  };

  const title = useMemo(() => (qFromUrl ? `搜索：${qFromUrl}` : "搜索文章"), [qFromUrl]);

  return (
    <main className="container">
      <section className="search-page-header">
        <div>
          <h1>{title}</h1>
          <p className="search-subtitle">支持标题与正文全文搜索（SQLite FTS5）</p>
        </div>
        <form className="search-inline" onSubmit={onSubmit}>
          <input
            value={input}
            onChange={(e) => setInput(e.target.value)}
            placeholder="输入关键词"
            maxLength={100}
          />
          <button type="submit">搜索</button>
        </form>
      </section>

      {loading ? <p className="status-line">正在搜索...</p> : null}
      {error ? <p className="status-line error">{error}</p> : null}

      {!qFromUrl ? <p className="empty-state">请输入关键词后开始搜索。</p> : null}
      {qFromUrl && !loading && items.length === 0 ? <p className="empty-state">没有匹配结果。</p> : null}

      <section className="post-list">
        {items.map((post) => (
          <PostCard key={post.id} post={post} />
        ))}
      </section>

      {qFromUrl ? <Pagination page={page} pageSize={10} total={total} onPageChange={onPageChange} /> : null}
    </main>
  );
}
