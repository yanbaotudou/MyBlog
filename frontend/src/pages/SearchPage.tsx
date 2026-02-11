import { useEffect, useMemo, useState } from "react";
import { ChevronLeft, ChevronRight, Clock, Search, X } from "lucide-react";
import { useNavigate, useSearchParams } from "react-router-dom";
import { searchPosts } from "../api/search";
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

function highlightText(text: string, query: string): string {
  if (!query) {
    return text;
  }
  const safeQuery = query.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const parts = text.split(new RegExp(`(${safeQuery})`, "gi"));
  return parts
    .map((part) =>
      part.toLowerCase() === query.toLowerCase() ? `<mark class="search-highlight">${part}</mark>` : part
    )
    .join("");
}

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

  const totalPages = useMemo(() => Math.max(1, Math.ceil(total / 10)), [total]);

  return (
    <div className="min-h-screen bg-background">
      <section className="sticky top-16 z-30 bg-background/80 backdrop-blur-md border-b border-border py-6">
        <div className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="relative">
            <Search className="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-5 text-muted-foreground" />
            <Input
              type="text"
              placeholder="输入关键词搜索文章..."
              value={input}
              onChange={(e) => setInput(e.target.value)}
              className="pl-12 pr-12 py-6 text-lg rounded-xl"
              onKeyDown={(e) => {
                if (e.key === "Enter") {
                  navigate(`/search?q=${encodeURIComponent(input.trim())}&page=1`);
                }
              }}
            />
            {input ? (
              <Button
                variant="ghost"
                size="icon"
                className="absolute right-2 top-1/2 -translate-y-1/2"
                onClick={() => {
                  setInput("");
                  navigate("/search");
                }}
              >
                <X className="w-5 h-5" />
              </Button>
            ) : null}
          </div>

          <div className="mt-4 flex items-center justify-between">
            <p className="text-sm text-muted-foreground">
              找到 <span className="font-medium text-foreground">{total}</span> 条结果
              {qFromUrl ? (
                <span>
                  {" "}
                  for "<span className="font-medium text-foreground">{qFromUrl}</span>"
                </span>
              ) : null}
            </p>
            <Badge variant="secondary">文章</Badge>
          </div>
        </div>
      </section>

      <section className="py-8">
        <div className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8">
          {loading ? <p className="text-sm text-muted-foreground">正在搜索...</p> : null}
          {error ? <p className="text-sm text-destructive">{error}</p> : null}

          {items.length > 0 ? (
            <>
              <div className="space-y-4">
                {items.map((post) => (
                  <Card key={post.id} className="card-hover cursor-pointer" onClick={() => navigate(`/posts/${post.id}`)}>
                    <CardContent className="p-6">
                      <h3
                        className="text-lg font-semibold text-foreground mb-2 hover:text-primary transition-colors"
                        dangerouslySetInnerHTML={{ __html: highlightText(post.title, qFromUrl) }}
                      />
                      <p
                        className="text-muted-foreground text-sm mb-4 line-clamp-2"
                        dangerouslySetInnerHTML={{ __html: highlightText(excerpt(post.contentMarkdown), qFromUrl) }}
                      />

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
                          <Clock className="w-3.5 h-3.5" />
                          {formatAbsoluteDateTime(post.updatedAt)}
                        </div>
                      </div>
                    </CardContent>
                  </Card>
                ))}
              </div>

              {totalPages > 1 ? (
                <div className="flex items-center justify-center gap-2 mt-8">
                  <Button
                    variant="outline"
                    size="icon"
                    disabled={page <= 1}
                    onClick={() => navigate(`/search?q=${encodeURIComponent(qFromUrl)}&page=${Math.max(1, page - 1)}`)}
                  >
                    <ChevronLeft className="w-4 h-4" />
                  </Button>
                  {Array.from({ length: totalPages }, (_, i) => i + 1)
                    .slice(Math.max(page - 3, 0), Math.max(page + 2, 5))
                    .map((n) => (
                      <Button
                        key={n}
                        variant={n === page ? "default" : "outline"}
                        size="sm"
                        onClick={() => navigate(`/search?q=${encodeURIComponent(qFromUrl)}&page=${n}`)}
                      >
                        {n}
                      </Button>
                    ))}
                  <Button
                    variant="outline"
                    size="icon"
                    disabled={page >= totalPages}
                    onClick={() => navigate(`/search?q=${encodeURIComponent(qFromUrl)}&page=${Math.min(totalPages, page + 1)}`)}
                  >
                    <ChevronRight className="w-4 h-4" />
                  </Button>
                </div>
              ) : null}
            </>
          ) : !loading ? (
            <div className="text-center py-16">
              <div className="w-16 h-16 bg-muted rounded-full flex items-center justify-center mx-auto mb-4">
                <Search className="w-8 h-8 text-muted-foreground" />
              </div>
              <h3 className="text-lg font-medium text-foreground mb-2">{qFromUrl ? "未找到相关文章" : "请输入关键词"}</h3>
              <p className="text-muted-foreground">{qFromUrl ? "请尝试使用其他关键词搜索" : "支持标题和正文全文搜索"}</p>
            </div>
          ) : null}
        </div>
      </section>
    </div>
  );
}
