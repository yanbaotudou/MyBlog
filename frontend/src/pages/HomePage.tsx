import { useEffect, useMemo, useState } from "react";
import { ChevronLeft, ChevronRight, Clock, Lock, PenLine, Search } from "lucide-react";
import { Link, useNavigate } from "react-router-dom";
import { listPosts } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import { Input } from "@/components/ui/input";

function excerpt(input: string): string {
  const stripped = input.replace(/[#>*`\-\[\]()]/g, " ").replace(/\s+/g, " ").trim();
  if (stripped.length <= 160) {
    return stripped;
  }
  return `${stripped.slice(0, 160)}...`;
}

export function HomePage() {
  const auth = useAuthState();
  const navigate = useNavigate();
  const [searchQuery, setSearchQuery] = useState("");
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

  const totalPages = useMemo(() => Math.max(1, Math.ceil(total / pageSize)), [total, pageSize]);

  return (
    <div className="min-h-screen bg-background">
      <section className="relative py-16 md:py-24 bg-gradient-to-b from-primary/5 to-background">
        <div className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8 text-center">
          <h1 className="text-3xl md:text-4xl font-bold text-foreground mb-4 animate-fade-in-up">
            发现优质技术文章
          </h1>
          <p className="text-muted-foreground mb-8 animate-fade-in-up stagger-1">
            探索、学习、分享你的技术见解
          </p>

          <div className="relative max-w-xl mx-auto animate-fade-in-up stagger-2">
            <Search className="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-5 text-muted-foreground" />
            <Input
              type="text"
              placeholder="搜索文章标题或内容..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="pl-12 pr-4 py-6 text-lg rounded-xl border-2 border-transparent focus:border-primary/30 shadow-lg"
              onKeyDown={(e) => {
                if (e.key === "Enter" && searchQuery.trim()) {
                  navigate(`/search?q=${encodeURIComponent(searchQuery.trim())}&page=1`);
                }
              }}
            />
            <Button
              className="absolute right-2 top-1/2 -translate-y-1/2 btn-hover-lift"
              onClick={() => navigate(`/search?q=${encodeURIComponent(searchQuery.trim())}&page=1`)}
              disabled={!searchQuery.trim()}
            >
              搜索
            </Button>
          </div>
        </div>
      </section>

      <section className="py-12">
        <div className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex items-center justify-between mb-8">
            <h2 className="text-xl font-semibold text-foreground">最新文章</h2>
            <span className="text-sm text-muted-foreground">共 {total} 篇</span>
          </div>

          {loading ? <p className="text-sm text-muted-foreground">加载中...</p> : null}
          {error ? <p className="text-sm text-destructive">{error}</p> : null}

          <div className="space-y-4">
            {items.map((post, index) => (
              <Card
                key={post.id}
                className="card-hover cursor-pointer opacity-0 animate-fade-in-up"
                style={{ animationDelay: `${(index + 1) * 0.08}s`, animationFillMode: "forwards" }}
                onClick={() => navigate(`/posts/${post.id}`)}
              >
                <CardContent className="p-6">
                  <div className="flex items-start justify-between gap-4">
                    <div className="flex-1 min-w-0">
                      <h3 className="text-lg font-semibold text-foreground mb-2 line-clamp-1 hover:text-primary transition-colors">
                        {post.title}
                      </h3>
                      <p className="text-muted-foreground text-sm mb-4 line-clamp-2">{excerpt(post.contentMarkdown)}</p>

                      <div className="flex items-center gap-4 flex-wrap">
                        <div className="flex items-center gap-2">
                          <Avatar className="w-6 h-6">
                            <AvatarFallback className="bg-primary/10 text-primary text-xs">
                              {post.authorUsername.charAt(0)}
                            </AvatarFallback>
                          </Avatar>
                          <span className="text-sm text-muted-foreground">{post.authorUsername}</span>
                        </div>

                        <div className="flex items-center gap-1 text-sm text-muted-foreground">
                          <Clock className="w-4 h-4" />
                          {formatAbsoluteDateTime(post.createdAt)}
                        </div>
                      </div>

                      <div className="flex gap-2 mt-3">
                        <Badge variant="secondary" className="text-xs">
                          发布时间
                        </Badge>
                        <Badge variant="secondary" className="text-xs">
                          {formatAbsoluteDateTime(post.updatedAt)}
                        </Badge>
                      </div>
                    </div>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>

          {!loading && items.length === 0 ? (
            <div className="text-center py-16">
              <div className="w-16 h-16 bg-muted rounded-full flex items-center justify-center mx-auto mb-4">
                <PenLine className="w-8 h-8 text-muted-foreground" />
              </div>
              <h3 className="text-lg font-medium text-foreground mb-2">暂无文章</h3>
              <p className="text-muted-foreground mb-4">来写第一篇吧！</p>
              {auth.user ? <Button onClick={() => navigate("/editor/new")}>新建文章</Button> : null}
            </div>
          ) : null}

          {totalPages > 1 ? (
            <div className="flex items-center justify-center gap-2 mt-8">
              <Button
                variant="outline"
                size="icon"
                disabled={page <= 1}
                onClick={() => setPage((p) => Math.max(1, p - 1))}
              >
                <ChevronLeft className="w-4 h-4" />
              </Button>
              {Array.from({ length: totalPages }, (_, i) => i + 1).slice(Math.max(page - 3, 0), Math.max(page + 2, 5)).map((n) => (
                <Button key={n} variant={n === page ? "default" : "outline"} size="sm" onClick={() => setPage(n)}>
                  {n}
                </Button>
              ))}
              <Button
                variant="outline"
                size="icon"
                disabled={page >= totalPages}
                onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
              >
                <ChevronRight className="w-4 h-4" />
              </Button>
            </div>
          ) : null}
        </div>
      </section>

      <div className="fixed bottom-8 right-8 z-40">
        {auth.user ? (
          <Link to="/editor/new">
            <Button size="lg" className="rounded-full shadow-lg btn-hover-lift gap-2">
              <PenLine className="w-5 h-5" />
              新建文章
            </Button>
          </Link>
        ) : (
          <Link to="/login">
            <Button size="lg" className="rounded-full shadow-lg btn-hover-lift gap-2">
              <Lock className="w-4 h-4" />
              新建文章（需登录）
            </Button>
          </Link>
        )}
      </div>
    </div>
  );
}
