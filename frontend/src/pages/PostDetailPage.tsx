import { useEffect, useMemo, useState } from "react";
import ReactMarkdown from "react-markdown";
import {
  ArrowLeft,
  Bookmark,
  Calendar,
  ChevronLeft,
  ChevronRight,
  Clock,
  Edit,
  FolderPlus,
  Heart,
  Link as LinkIcon,
  List,
  MessageSquare,
  Share2,
  Trash2
} from "lucide-react";
import { Link, useNavigate, useParams, useSearchParams } from "react-router-dom";
import {
  createComment,
  deleteComment,
  favoritePost,
  getPostInteractions,
  likePost,
  listComments,
  unfavoritePost,
  unlikePost
} from "../api/interactions";
import { addPostToCollection, getPostCollections, listMyCollections } from "../api/collections";
import { deletePost, getPost } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { Collection, CollectionNavigation, PostCollectionMembership } from "../types/collection";
import type { CommentItem, InteractionSummary } from "../types/interaction";
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
  DropdownMenuTrigger
} from "@/components/ui/dropdown-menu";
import { Textarea } from "@/components/ui/textarea";

const COMMENT_PAGE_SIZE = 10;

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
  const [collectionsError, setCollectionsError] = useState("");

  const [myCollections, setMyCollections] = useState<Collection[]>([]);
  const [selectedAddCollectionId, setSelectedAddCollectionId] = useState<number>(0);
  const [addingToCollection, setAddingToCollection] = useState(false);
  const [showAddDialog, setShowAddDialog] = useState(false);

  const [interactionSummary, setInteractionSummary] = useState<InteractionSummary | null>(null);
  const [interactionsError, setInteractionsError] = useState("");
  const [interactionPending, setInteractionPending] = useState<"like" | "favorite" | null>(null);

  const [comments, setComments] = useState<CommentItem[]>([]);
  const [commentTotal, setCommentTotal] = useState(0);
  const [commentPage, setCommentPage] = useState(1);
  const [commentsLoading, setCommentsLoading] = useState(false);
  const [commentError, setCommentError] = useState("");
  const [commentInput, setCommentInput] = useState("");
  const [commentSubmitting, setCommentSubmitting] = useState(false);
  const [deletingCommentId, setDeletingCommentId] = useState<number | null>(null);

  const selectedCollectionId = (() => {
    const raw = searchParams.get("collectionId");
    if (!raw) return undefined;
    const id = Number(raw);
    return Number.isFinite(id) && id > 0 ? id : undefined;
  })();

  const commentPageCount = useMemo(() => Math.max(1, Math.ceil(commentTotal / COMMENT_PAGE_SIZE)), [commentTotal]);

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

  const loadInteractions = async (postId: number) => {
    setInteractionsError("");
    try {
      const data = await getPostInteractions(postId);
      setInteractionSummary(data);
    } catch (e) {
      setInteractionsError(e instanceof Error ? e.message : "加载互动数据失败");
    }
  };

  const loadComments = async (postId: number, page: number) => {
    setCommentsLoading(true);
    setCommentError("");
    try {
      const data = await listComments(postId, page, COMMENT_PAGE_SIZE);
      setComments(data.items);
      setCommentTotal(data.total);
      setCommentPage(data.page);
    } catch (e) {
      setCommentError(e instanceof Error ? e.message : "加载评论失败");
    } finally {
      setCommentsLoading(false);
    }
  };

  useEffect(() => {
    if (!post) return;
    setCommentPage(1);
    setCommentInput("");
    setComments([]);
    setCommentTotal(0);
  }, [post?.id]);

  useEffect(() => {
    if (!post) return;
    const run = async () => {
      setCollectionsError("");
      try {
        const data = await getPostCollections(post.id, selectedCollectionId);
        setMemberships(data.items);
        setNavigation(data.navigation);
      } catch (e) {
        setCollectionsError(e instanceof Error ? e.message : "加载合集信息失败");
        setNavigation(null);
      }
    };
    void run();
  }, [post, selectedCollectionId]);

  useEffect(() => {
    if (!post) return;
    void loadInteractions(post.id);
  }, [post, auth.user?.id]);

  useEffect(() => {
    if (!post) return;
    void loadComments(post.id, commentPage);
  }, [post, commentPage]);

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

  const ensureLoginForInteraction = (): boolean => {
    if (auth.user) {
      return true;
    }
    setInteractionsError("请先登录后再进行互动");
    navigate("/login");
    return false;
  };

  const handleLikeToggle = async () => {
    if (!post || !interactionSummary || !ensureLoginForInteraction()) return;
    setInteractionPending("like");
    setInteractionsError("");
    try {
      const data = interactionSummary.likedByMe ? await unlikePost(post.id) : await likePost(post.id);
      setInteractionSummary(data);
    } catch (e) {
      setInteractionsError(e instanceof Error ? e.message : "点赞操作失败");
    } finally {
      setInteractionPending(null);
    }
  };

  const handleFavoriteToggle = async () => {
    if (!post || !interactionSummary || !ensureLoginForInteraction()) return;
    setInteractionPending("favorite");
    setInteractionsError("");
    try {
      const data = interactionSummary.favoritedByMe ? await unfavoritePost(post.id) : await favoritePost(post.id);
      setInteractionSummary(data);
    } catch (e) {
      setInteractionsError(e instanceof Error ? e.message : "收藏操作失败");
    } finally {
      setInteractionPending(null);
    }
  };

  const handleSubmitComment = async () => {
    if (!post || !ensureLoginForInteraction()) return;
    const content = commentInput.trim();
    if (!content) {
      setCommentError("评论内容不能为空");
      return;
    }
    setCommentSubmitting(true);
    setCommentError("");
    try {
      await createComment(post.id, content);
      setCommentInput("");
      await loadInteractions(post.id);
      await loadComments(post.id, 1);
      setCommentPage(1);
    } catch (e) {
      setCommentError(e instanceof Error ? e.message : "发表评论失败");
    } finally {
      setCommentSubmitting(false);
    }
  };

  const handleDeleteComment = async (comment: CommentItem) => {
    if (!post) return;
    if (!window.confirm("确认删除这条评论吗？")) return;
    setDeletingCommentId(comment.id);
    setCommentError("");
    try {
      await deleteComment(comment.id);
      await loadInteractions(post.id);
      if (comments.length === 1 && commentPage > 1) {
        setCommentPage((prev) => Math.max(1, prev - 1));
      } else {
        await loadComments(post.id, commentPage);
      }
    } catch (e) {
      setCommentError(e instanceof Error ? e.message : "删除评论失败");
    } finally {
      setDeletingCommentId(null);
    }
  };

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

  const likeCount = interactionSummary?.likeCount ?? 0;
  const favoriteCount = interactionSummary?.favoriteCount ?? 0;
  const commentsCount = interactionSummary?.commentCount ?? commentTotal;

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

        <div className="flex gap-2 mb-8 flex-wrap">
          <Badge variant="secondary">文章</Badge>
          {selectedCollectionId ? <Badge variant="secondary">合集上下文</Badge> : null}
        </div>

        <div className="markdown-content">
          <ReactMarkdown>{post.contentMarkdown}</ReactMarkdown>
        </div>

        <section className="mt-10 border-t border-border pt-6 space-y-4">
          <div className="flex items-center gap-2 flex-wrap">
            <Button
              type="button"
              variant={interactionSummary?.likedByMe ? "default" : "outline"}
              size="sm"
              className="gap-2"
              disabled={interactionPending !== null}
              onClick={handleLikeToggle}
            >
              <Heart className={`w-4 h-4 ${interactionSummary?.likedByMe ? "fill-current" : ""}`} />
              点赞 {likeCount}
            </Button>

            <Button
              type="button"
              variant={interactionSummary?.favoritedByMe ? "default" : "outline"}
              size="sm"
              className="gap-2"
              disabled={interactionPending !== null}
              onClick={handleFavoriteToggle}
            >
              <Bookmark className={`w-4 h-4 ${interactionSummary?.favoritedByMe ? "fill-current" : ""}`} />
              收藏 {favoriteCount}
            </Button>

            <Badge variant="outline" className="h-8 px-3 text-sm gap-1 inline-flex items-center">
              <MessageSquare className="w-3.5 h-3.5" />
              评论 {commentsCount}
            </Badge>
          </div>

          {interactionsError ? <p className="text-xs text-destructive">{interactionsError}</p> : null}
          {shareMessage ? <p className="text-xs text-muted-foreground">{shareMessage}</p> : null}
          {collectionsError ? <p className="text-xs text-destructive">{collectionsError}</p> : null}
        </section>

        <section className="mt-8 space-y-4">
          <h2 className="text-xl font-semibold">评论</h2>

          {auth.user ? (
            <div className="space-y-2">
              <Textarea
                placeholder="写下你的评论（最多 2000 字）"
                value={commentInput}
                onChange={(e) => setCommentInput(e.target.value)}
                maxLength={2000}
                rows={4}
              />
              <div className="flex items-center justify-between">
                <p className="text-xs text-muted-foreground">{commentInput.length}/2000</p>
                <Button type="button" size="sm" onClick={handleSubmitComment} disabled={commentSubmitting}>
                  {commentSubmitting ? "提交中..." : "发表评论"}
                </Button>
              </div>
            </div>
          ) : (
            <p className="text-sm text-muted-foreground">
              <Link to="/login" className="text-primary hover:underline">
                登录
              </Link>
              后可发表评论。
            </p>
          )}

          {commentError ? <p className="text-sm text-destructive">{commentError}</p> : null}

          {commentsLoading ? <p className="text-sm text-muted-foreground">评论加载中...</p> : null}

          {!commentsLoading && comments.length === 0 ? (
            <p className="text-sm text-muted-foreground">暂无评论，来发第一条吧。</p>
          ) : null}

          <div className="space-y-3">
            {comments.map((comment) => {
              const canDeleteComment = !!auth.user && (auth.user.role === "admin" || auth.user.id === comment.userId);
              return (
                <div key={comment.id} className="rounded-lg border border-border p-4 space-y-2">
                  <div className="flex items-center justify-between gap-2">
                    <div className="flex items-center gap-2 text-sm">
                      <span className="font-medium text-foreground">{comment.username}</span>
                      <span className="text-muted-foreground">{formatAbsoluteDateTime(comment.createdAt)}</span>
                    </div>
                    {canDeleteComment ? (
                      <Button
                        type="button"
                        size="sm"
                        variant="ghost"
                        className="text-destructive hover:text-destructive"
                        onClick={() => void handleDeleteComment(comment)}
                        disabled={deletingCommentId === comment.id}
                      >
                        删除
                      </Button>
                    ) : null}
                  </div>
                  <p className="text-sm leading-6 whitespace-pre-wrap break-words">{comment.content}</p>
                </div>
              );
            })}
          </div>

          {commentPageCount > 1 ? (
            <div className="flex items-center justify-between pt-2">
              <p className="text-xs text-muted-foreground">
                第 {commentPage} / {commentPageCount} 页，共 {commentTotal} 条评论
              </p>
              <div className="flex gap-2">
                <Button
                  type="button"
                  variant="outline"
                  size="sm"
                  disabled={commentPage <= 1 || commentsLoading}
                  onClick={() => setCommentPage((prev) => Math.max(1, prev - 1))}
                >
                  上一页
                </Button>
                <Button
                  type="button"
                  variant="outline"
                  size="sm"
                  disabled={commentPage >= commentPageCount || commentsLoading}
                  onClick={() => setCommentPage((prev) => Math.min(commentPageCount, prev + 1))}
                >
                  下一页
                </Button>
              </div>
            </div>
          ) : null}
        </section>

        {selectedCollectionId && navigation ? (
          <div className="bg-secondary/50 rounded-xl p-6 mt-10">
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
