import { FormEvent, useEffect, useMemo, useState } from "react";
import { Helmet } from "react-helmet-async";
import {
  ArrowLeft,
  Clock,
  ExternalLink,
  Globe,
  GripVertical,
  Link as LinkIcon,
  List,
  Lock,
  MoreHorizontal,
  Plus,
  Search,
  Share2,
  User
} from "lucide-react";
import { Link, useNavigate, useParams } from "react-router-dom";
import { addPostToCollection, getCollection, removePostFromCollection } from "../api/collections";
import { listPosts } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { CollectionDetail } from "../types/collection";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle
} from "@/components/ui/dialog";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger
} from "@/components/ui/dropdown-menu";
import { Input } from "@/components/ui/input";

function resolveSiteUrl(): string {
  const envUrl = import.meta.env.VITE_SITE_URL;
  if (typeof envUrl === "string" && envUrl.trim()) {
    return envUrl.trim().replace(/\/+$/, "");
  }
  if (typeof window !== "undefined") {
    return window.location.origin;
  }
  return "";
}

function absoluteUrl(siteUrl: string, path: string): string {
  if (!siteUrl) {
    return path;
  }
  return `${siteUrl}${path.startsWith("/") ? path : `/${path}`}`;
}

async function copyText(text: string): Promise<boolean> {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    return false;
  }
}

export function CollectionDetailPage() {
  const params = useParams();
  const navigate = useNavigate();
  const auth = useAuthState();
  const [detail, setDetail] = useState<CollectionDetail | null>(null);
  const [allPosts, setAllPosts] = useState<Post[]>([]);
  const [searchQuery, setSearchQuery] = useState("");
  const [showAddArticle, setShowAddArticle] = useState(false);
  const [shareMessage, setShareMessage] = useState("");
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const siteUrl = useMemo(resolveSiteUrl, []);

  const collectionId = Number(params.id);

  const loadData = async () => {
    if (!collectionId) {
      setError("合集 ID 无效");
      return;
    }

    setLoading(true);
    setError("");
    try {
      const collectionData = await getCollection(collectionId);
      setDetail(collectionData);

      if (
        auth.user &&
        (auth.user.role === "admin" || auth.user.id === collectionData.collection.ownerId)
      ) {
        const postsData = await listPosts(1, 200);
        setAllPosts(postsData.items);
      } else {
        setAllPosts([]);
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : "加载合集失败");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    void loadData();
  }, [collectionId, auth.user?.id, auth.user?.role]);

  const canEdit = useMemo(() => {
    if (!detail || !auth.user) {
      return false;
    }
    return auth.user.role === "admin" || auth.user.id === detail.collection.ownerId;
  }, [detail, auth.user]);

  const availablePosts = useMemo(() => {
    if (!detail) {
      return [];
    }
    const existing = new Set(detail.posts.map((post) => post.id));
    return allPosts.filter((post) => !existing.has(post.id));
  }, [allPosts, detail]);

  const filteredAvailablePosts = useMemo(() => {
    if (!searchQuery.trim()) {
      return availablePosts;
    }
    const q = searchQuery.toLowerCase();
    return availablePosts.filter((post) => post.title.toLowerCase().includes(q));
  }, [availablePosts, searchQuery]);

  const canonicalUrl = useMemo(
    () => absoluteUrl(siteUrl, `/collections/${collectionId}`),
    [siteUrl, collectionId]
  );
  const ogImage = useMemo(() => absoluteUrl(siteUrl, "/og-cover.svg"), [siteUrl]);

  const metaTitle = detail ? `${detail.collection.name} | 学习合集` : "学习合集 | 学习日志";
  const metaDescription = detail
    ? (detail.collection.description || `${detail.collection.ownerUsername} 创建的学习合集，共 ${detail.total} 篇文章。`)
        .replace(/\s+/g, " ")
        .slice(0, 160)
    : "学习日志博客合集页面，持续整理学习路径。";

  const onRemovePost = async (postId: number) => {
    if (!detail) return;
    if (!window.confirm("确认从合集中移除该文章吗？")) return;
    setSaving(true);
    try {
      await removePostFromCollection(detail.collection.id, postId);
      await loadData();
    } catch (e) {
      setError(e instanceof Error ? e.message : "移除失败");
    } finally {
      setSaving(false);
    }
  };

  const onAddPost = async (postId: number) => {
    if (!detail) return;
    setSaving(true);
    try {
      await addPostToCollection(detail.collection.id, postId);
      setShowAddArticle(false);
      setSearchQuery("");
      await loadData();
    } catch (e) {
      setError(e instanceof Error ? e.message : "添加失败");
    } finally {
      setSaving(false);
    }
  };

  const onCopyLink = async () => {
    const ok = await copyText(canonicalUrl);
    setShareMessage(ok ? "链接已复制到剪贴板" : "复制失败");
  };

  const onCopyOutline = async () => {
    if (!detail) return;
    const outline = [
      `# ${detail.collection.name}`,
      "",
      ...detail.posts.map((a, i) => `${i + 1}. ${a.title}`)
    ].join("\n");
    const ok = await copyText(outline);
    setShareMessage(ok ? "目录已复制到剪贴板" : "复制失败");
  };

  const onShareWeibo = () => {
    window.open(
      `https://service.weibo.com/share/share.php?url=${encodeURIComponent(
        canonicalUrl
      )}&title=${encodeURIComponent(metaTitle)}`,
      "_blank"
    );
  };

  const onShareX = () => {
    window.open(
      `https://x.com/intent/tweet?url=${encodeURIComponent(canonicalUrl)}&text=${encodeURIComponent(metaTitle)}`,
      "_blank"
    );
  };

  const onNativeShare = async () => {
    if (!navigator.share) {
      setShareMessage("当前浏览器不支持系统分享");
      return;
    }
    await navigator.share({
      title: metaTitle,
      text: metaDescription,
      url: canonicalUrl
    });
  };

  const seoLdJson = useMemo(() => {
    if (!detail) {
      return "";
    }
    return JSON.stringify({
      "@context": "https://schema.org",
      "@type": "CollectionPage",
      name: detail.collection.name,
      description: metaDescription,
      url: canonicalUrl,
      dateModified: detail.collection.updatedAt,
      creator: { "@type": "Person", name: detail.collection.ownerUsername },
      mainEntity: {
        "@type": "ItemList",
        itemListElement: detail.posts.map((post, idx) => ({
          "@type": "ListItem",
          position: idx + 1,
          name: post.title,
          url: absoluteUrl(siteUrl, `/posts/${post.id}?collectionId=${detail.collection.id}`)
        }))
      }
    });
  }, [detail, metaDescription, canonicalUrl, siteUrl]);

  if (loading) {
    return <div className="min-h-screen flex items-center justify-center text-muted-foreground">正在加载合集...</div>;
  }
  if (!detail) {
    return <div className="min-h-screen flex items-center justify-center text-destructive">{error || "合集不存在"}</div>;
  }

  return (
    <div className="min-h-screen bg-background">
      <Helmet>
        <title>{metaTitle}</title>
        <meta name="description" content={metaDescription} />
        <meta property="og:type" content="website" />
        <meta property="og:title" content={metaTitle} />
        <meta property="og:description" content={metaDescription} />
        <meta property="og:url" content={canonicalUrl} />
        <meta property="og:image" content={ogImage} />
        <meta name="twitter:card" content="summary_large_image" />
        <meta name="twitter:title" content={metaTitle} />
        <meta name="twitter:description" content={metaDescription} />
        <meta name="twitter:image" content={ogImage} />
        <link rel="canonical" href={canonicalUrl} />
        <script type="application/ld+json">{seoLdJson}</script>
      </Helmet>

      <div className="border-b border-border">
        <div className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
          <Button variant="ghost" size="sm" className="gap-2 mb-6" onClick={() => navigate("/collections")}>
            <ArrowLeft className="w-4 h-4" />
            返回合集列表
          </Button>

          <div className="flex items-start justify-between gap-4">
            <div className="flex-1">
              <div className="flex items-center gap-3 mb-3">
                <h1 className="text-3xl font-bold text-foreground">{detail.collection.name}</h1>
                <Badge variant="secondary" className="gap-1">
                  {true ? <Globe className="w-3 h-3" /> : <Lock className="w-3 h-3" />}
                  公开
                </Badge>
              </div>
              <p className="text-muted-foreground text-lg">{detail.collection.description || "暂无描述"}</p>

              <div className="flex items-center gap-4 mt-4 text-sm text-muted-foreground">
                <div className="flex items-center gap-2">
                  <Avatar className="w-6 h-6">
                    <AvatarFallback className="bg-primary/10 text-primary text-xs">
                      {detail.collection.ownerUsername.charAt(0)}
                    </AvatarFallback>
                  </Avatar>
                  <span>{detail.collection.ownerUsername}</span>
                </div>
                <span>·</span>
                <span>{detail.total} 篇文章</span>
                <span>·</span>
                <span>更新于 {formatAbsoluteDateTime(detail.collection.updatedAt)}</span>
              </div>
            </div>

            <div className="flex items-center gap-2">
              <DropdownMenu>
                <DropdownMenuTrigger asChild>
                  <Button variant="outline" size="sm" className="gap-2">
                    <Share2 className="w-4 h-4" />
                    分享
                  </Button>
                </DropdownMenuTrigger>
                <DropdownMenuContent align="end">
                  <DropdownMenuItem onClick={onCopyLink}>
                    <LinkIcon className="mr-2 h-4 w-4" />
                    复制链接
                  </DropdownMenuItem>
                  <DropdownMenuItem onClick={onCopyOutline}>
                    <List className="mr-2 h-4 w-4" />
                    复制目录
                  </DropdownMenuItem>
                  <DropdownMenuSeparator />
                  <DropdownMenuItem onClick={onNativeShare}>系统分享</DropdownMenuItem>
                  <DropdownMenuItem onClick={onShareWeibo}>分享到微博</DropdownMenuItem>
                  <DropdownMenuItem onClick={onShareX}>分享到 X</DropdownMenuItem>
                </DropdownMenuContent>
              </DropdownMenu>

              {canEdit ? (
                <Button variant="outline" size="sm" className="gap-2" onClick={() => setShowAddArticle(true)}>
                  <Plus className="w-4 h-4" />
                  添加文章
                </Button>
              ) : null}
            </div>
          </div>

          {shareMessage ? <p className="text-xs text-muted-foreground mt-3">{shareMessage}</p> : null}
          {error ? <p className="text-xs text-destructive mt-2">{error}</p> : null}
        </div>
      </div>

      <div className="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="space-y-4">
          {detail.posts.map((article, index) => (
            <Card key={article.id} className="card-hover cursor-pointer group" onClick={() => navigate(`/posts/${article.id}?collectionId=${detail.collection.id}`)}>
              <CardContent className="p-6">
                <div className="flex items-start gap-4">
                  {canEdit ? (
                    <div className="pt-1 cursor-grab active:cursor-grabbing">
                      <GripVertical className="w-5 h-5 text-muted-foreground" />
                    </div>
                  ) : null}
                  <div className="w-8 h-8 bg-primary/10 rounded-lg flex items-center justify-center shrink-0">
                    <span className="text-sm font-semibold text-primary">{index + 1}</span>
                  </div>
                  <div className="flex-1 min-w-0">
                    <h3 className="text-lg font-semibold text-foreground mb-2 line-clamp-1 group-hover:text-primary transition-colors">
                      {article.title}
                    </h3>
                    <p className="text-muted-foreground text-sm mb-3 line-clamp-2">{article.contentMarkdown}</p>
                    <div className="flex items-center gap-4 text-sm text-muted-foreground">
                      <span className="flex items-center gap-1">
                        <User className="w-4 h-4" />
                        {article.authorUsername}
                      </span>
                      <span className="flex items-center gap-1">
                        <Clock className="w-4 h-4" />
                        {formatAbsoluteDateTime(article.updatedAt)}
                      </span>
                    </div>
                  </div>

                  {canEdit ? (
                    <DropdownMenu>
                      <DropdownMenuTrigger asChild>
                        <Button variant="ghost" size="icon" className="opacity-0 group-hover:opacity-100 transition-opacity" onClick={(e) => e.stopPropagation()}>
                          <MoreHorizontal className="w-4 h-4" />
                        </Button>
                      </DropdownMenuTrigger>
                      <DropdownMenuContent align="end">
                        <DropdownMenuItem
                          className="text-destructive"
                          onClick={(e) => {
                            e.stopPropagation();
                            void onRemovePost(article.id);
                          }}
                        >
                          从合集移除
                        </DropdownMenuItem>
                        <DropdownMenuItem
                          onClick={(e) => {
                            e.stopPropagation();
                            navigate(`/posts/${article.id}?collectionId=${detail.collection.id}`);
                          }}
                        >
                          <ExternalLink className="mr-2 h-4 w-4" />
                          打开文章
                        </DropdownMenuItem>
                      </DropdownMenuContent>
                    </DropdownMenu>
                  ) : null}
                </div>
              </CardContent>
            </Card>
          ))}
        </div>
      </div>

      <Dialog open={showAddArticle} onOpenChange={setShowAddArticle}>
        <DialogContent className="max-w-lg">
          <DialogHeader>
            <DialogTitle>添加文章到合集</DialogTitle>
            <DialogDescription>搜索并选择要添加的文章</DialogDescription>
          </DialogHeader>
          <div className="space-y-4 py-4">
            <div className="relative">
              <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
              <Input
                placeholder="搜索文章..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="pl-10"
              />
            </div>
            <div className="space-y-2 max-h-64 overflow-y-auto">
              {filteredAvailablePosts.map((article) => (
                <div
                  key={article.id}
                  className="flex items-center justify-between p-3 rounded-lg border hover:bg-secondary cursor-pointer transition-colors"
                >
                  <div>
                    <p className="font-medium text-sm">{article.title}</p>
                    <p className="text-xs text-muted-foreground line-clamp-1">{article.contentMarkdown}</p>
                  </div>
                  <Button size="sm" onClick={() => void onAddPost(article.id)} disabled={saving}>
                    添加
                  </Button>
                </div>
              ))}
              {filteredAvailablePosts.length === 0 ? (
                <p className="text-center text-sm text-muted-foreground py-4">暂无可添加文章</p>
              ) : null}
            </div>
          </div>
        </DialogContent>
      </Dialog>
    </div>
  );
}
