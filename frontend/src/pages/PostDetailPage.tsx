import { useEffect, useMemo, useState } from "react";
import ReactMarkdown from "react-markdown";
import {
  ArrowLeft,
  Calendar,
  ChevronLeft,
  ChevronRight,
  Clock,
  Edit,
  FolderPlus,
  Link as LinkIcon,
  List,
  Share2,
  Trash2
} from "lucide-react";
import { Link, useNavigate, useParams, useSearchParams } from "react-router-dom";
import { addPostToCollection, getPostCollections, listMyCollections } from "../api/collections";
import { deletePost, getPost } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { Collection, CollectionNavigation, PostCollectionMembership } from "../types/collection";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
  DialogTrigger
} from "@/components/ui/dialog";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger
} from "@/components/ui/dropdown-menu";

async function copyToClipboard(text: string): Promise<boolean> {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    return false;
  }
}

export function PostDetailPage() {
  const params = useParams();
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const auth = useAuthState();

  const [post, setPost] = useState<Post | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [deleting, setDeleting] = useState(false);
  const [shareMessage, setShareMessage] = useState("");

  const [memberships, setMemberships] = useState<PostCollectionMembership[]>([]);
  const [navigation, setNavigation] = useState<CollectionNavigation | null>(null);
  const [collectionsLoading, setCollectionsLoading] = useState(false);
  const [collectionsError, setCollectionsError] = useState("");

  const [myCollections, setMyCollections] = useState<Collection[]>([]);
  const [selectedAddCollectionId, setSelectedAddCollectionId] = useState<number>(0);
  const [addingToCollection, setAddingToCollection] = useState(false);
  const [showAddDialog, setShowAddDialog] = useState(false);

  const selectedCollectionId = (() => {
    const raw = searchParams.get("collectionId");
    if (!raw) return undefined;
    const id = Number(raw);
    return Number.isFinite(id) && id > 0 ? id : undefined;
  })();

  useEffect(() => {
    const id = Number(params.id);
    if (!id) {
      setError("文章 ID 无效");
      return;
    }
    const run = async () => {
      setLoading(true);
      setError("");
      try {
        const data = await getPost(id);
        setPost(data);
      } catch (e) {
        setError(e instanceof Error ? e.message : "加载失败");
      } finally {
        setLoading(false);
      }
    };
    void run();
  }, [params.id]);

  useEffect(() => {
    if (!post) return;
    const run = async () => {
      setCollectionsLoading(true);
      setCollectionsError("");
      try {
        const data = await getPostCollections(post.id, selectedCollectionId);
        setMemberships(data.items);
        setNavigation(data.navigation);
      } catch (e) {
        setCollectionsError(e instanceof Error ? e.message : "加载合集信息失败");
        setNavigation(null);
      } finally {
        setCollectionsLoading(false);
      }
    };
    void run();
  }, [post, selectedCollectionId]);

  useEffect(() => {
    if (!auth.user) {
      setMyCollections([]);
      return;
    }
    const run = async () => {
      try {
        const data = await listMyCollections();
        setMyCollections(data.items);
        if (data.items.length > 0) {
          setSelectedAddCollectionId(data.items[0].id);
        }
      } catch {
        setMyCollections([]);
      }
    };
    void run();
  }, [auth.user]);

  const canEdit = useMemo(() => {
    if (!post || !auth.user) return false;
    return auth.user.role === "admin" || auth.user.id === post.authorId;
  }, [post, auth.user]);

  const handleDelete = async () => {
    if (!post) return;
    if (!window.confirm("确认删除这篇文章吗？")) return;
    setDeleting(true);
    try {
      await deletePost(post.id);
      navigate("/");
    } catch (e) {
      setError(e instanceof Error ? e.message : "删除失败");
      setDeleting(false);
    }
  };

  const handleAddCurrentPostToCollection = async () => {
    if (!post || !selectedAddCollectionId) return;
    setAddingToCollection(true);
    setCollectionsError("");
    try {
      await addPostToCollection(selectedAddCollectionId, post.id);
      const data = await getPostCollections(post.id, selectedCollectionId);
      setMemberships(data.items);
      setNavigation(data.navigation);
      setShowAddDialog(false);
    } catch (e) {
      setCollectionsError(e instanceof Error ? e.message : "加入合集失败");
    } finally {
      setAddingToCollection(false);
    }
  };

  const onCopyLink = async () => {
    const ok = await copyToClipboard(window.location.href);
    setShareMessage(ok ? "链接已复制到剪贴板" : "复制失败");
  };

  const onCopyOutline = async () => {
    if (!post) return;
    const outline = `# ${post.title}\n\n1. 正文`;
    const ok = await copyToClipboard(outline);
    setShareMessage(ok ? "目录已复制到剪贴板" : "复制失败");
  };

  if (loading) {
    return <div className="min-h-screen flex items-center justify-center text-muted-foreground">正在加载文章...</div>;
  }
  if (error) {
    return <div className="min-h-screen flex items-center justify-center text-destructive">{error}</div>;
  }
  if (!post) {
    return <div className="min-h-screen flex items-center justify-center text-muted-foreground">文章不存在</div>;
  }

  return (
    <div className="min-h-screen bg-background">
      <div className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8 pt-6">
        <Button variant="ghost" size="sm" className="gap-2" onClick={() => navigate("/")}>
          <ArrowLeft className="w-4 h-4" />
          返回首页
        </Button>
      </div>

      <article className="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <h1 className="text-3xl md:text-4xl font-bold text-foreground mb-6">{post.title}</h1>

        <div className="flex items-center justify-between flex-wrap gap-4 mb-8">
          <div className="flex items-center gap-3">
            <Avatar className="w-10 h-10">
              <AvatarFallback className="bg-primary/10 text-primary">{post.authorUsername.charAt(0)}</AvatarFallback>
            </Avatar>
            <div>
              <p className="font-medium text-foreground">{post.authorUsername}</p>
              <div className="flex items-center gap-3 text-sm text-muted-foreground">
                <span className="flex items-center gap-1">
                  <Calendar className="w-3.5 h-3.5" />
                  发布于 {formatAbsoluteDateTime(post.createdAt)}
                </span>
                <span className="flex items-center gap-1">
                  <Clock className="w-3.5 h-3.5" />
                  更新于 {formatAbsoluteDateTime(post.updatedAt)}
                </span>
              </div>
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
              </DropdownMenuContent>
            </DropdownMenu>

            {auth.user && myCollections.length > 0 ? (
              <Dialog open={showAddDialog} onOpenChange={setShowAddDialog}>
                <DialogTrigger asChild>
                  <Button variant="outline" size="sm" className="gap-2">
                    <FolderPlus className="w-4 h-4" />
                    加入合集
                  </Button>
                </DialogTrigger>
                <DialogContent>
                  <DialogHeader>
                    <DialogTitle>加入合集</DialogTitle>
                    <DialogDescription>选择要将此文章加入的合集</DialogDescription>
                  </DialogHeader>
                  <div className="space-y-2 mt-4">
                    {myCollections.map((collection) => (
                      <div
                        key={collection.id}
                        className={`flex items-center justify-between p-3 rounded-lg border transition-colors ${
                          selectedAddCollectionId === collection.id ? "bg-secondary" : "hover:bg-secondary"
                        }`}
                        onClick={() => setSelectedAddCollectionId(collection.id)}
                      >
                        <div>
                          <p className="font-medium">{collection.name}</p>
                          <p className="text-sm text-muted-foreground">{collection.postCount} 篇文章</p>
                        </div>
                        <Button
                          size="sm"
                          variant="secondary"
                          onClick={(e) => {
                            e.stopPropagation();
                            setSelectedAddCollectionId(collection.id);
                            void handleAddCurrentPostToCollection();
                          }}
                          disabled={addingToCollection}
                        >
                          添加
                        </Button>
                      </div>
                    ))}
                  </div>
                </DialogContent>
              </Dialog>
            ) : null}

            {canEdit ? (
              <>
                <Button variant="outline" size="sm" className="gap-2" onClick={() => navigate(`/editor/${post.id}`)}>
                  <Edit className="w-4 h-4" />
                  编辑
                </Button>
                <Button variant="destructive" size="sm" className="gap-2" onClick={handleDelete} disabled={deleting}>
                  <Trash2 className="w-4 h-4" />
                  删除
                </Button>
              </>
            ) : null}
          </div>
        </div>

        <div className="flex gap-2 mb-8">
          <Badge variant="secondary">文章</Badge>
          {selectedCollectionId ? <Badge variant="secondary">合集上下文</Badge> : null}
        </div>

        <div className="markdown-content">
          <ReactMarkdown>{post.contentMarkdown}</ReactMarkdown>
        </div>

        {shareMessage ? <p className="text-xs text-muted-foreground mt-4">{shareMessage}</p> : null}
        {collectionsError ? <p className="text-xs text-destructive mt-2">{collectionsError}</p> : null}

        {selectedCollectionId && navigation ? (
          <div className="bg-secondary/50 rounded-xl p-6 mt-8">
            <p className="text-sm text-muted-foreground mb-4">
              来自合集：
              <span className="font-medium text-foreground">
                {memberships.find((m) => m.collectionId === selectedCollectionId)?.collectionName || `#${selectedCollectionId}`}
              </span>
            </p>
            <div className="flex items-center justify-between gap-4">
              {navigation.prev ? (
                <Link to={`/posts/${navigation.prev.id}?collectionId=${selectedCollectionId}`} className="flex-1">
                  <Button variant="ghost" className="w-full justify-start gap-2 h-auto py-3">
                    <ChevronLeft className="w-4 h-4 shrink-0" />
                    <div className="text-left">
                      <p className="text-xs text-muted-foreground">上一篇</p>
                      <p className="text-sm font-medium line-clamp-1">{navigation.prev.title}</p>
                    </div>
                  </Button>
                </Link>
              ) : (
                <div className="flex-1" />
              )}

              {navigation.next ? (
                <Link to={`/posts/${navigation.next.id}?collectionId=${selectedCollectionId}`} className="flex-1">
                  <Button variant="ghost" className="w-full justify-end gap-2 h-auto py-3">
                    <div className="text-right">
                      <p className="text-xs text-muted-foreground">下一篇</p>
                      <p className="text-sm font-medium line-clamp-1">{navigation.next.title}</p>
                    </div>
                    <ChevronRight className="w-4 h-4 shrink-0" />
                  </Button>
                </Link>
              ) : (
                <div className="flex-1" />
              )}
            </div>
          </div>
        ) : null}
      </article>
    </div>
  );
}
