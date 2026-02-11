import { useEffect, useMemo, useState } from "react";
import { BookOpen, FilePenLine, KeyRound, LogOut, Trash2, UserCircle2 } from "lucide-react";
import { Link, useNavigate } from "react-router-dom";
import { logout } from "../api/auth";
import { listMyCollections } from "../api/collections";
import { deletePost, listMyPosts } from "../api/posts";
import { authStore, useAuthState } from "../store/authStore";
import type { Collection } from "../types/collection";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Avatar, AvatarFallback } from "@/components/ui/avatar";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";

const PAGE_SIZE = 10;

export function ProfilePage() {
  const auth = useAuthState();
  const navigate = useNavigate();
  const [posts, setPosts] = useState<Post[]>([]);
  const [collections, setCollections] = useState<Collection[]>([]);
  const [page, setPage] = useState(1);
  const [total, setTotal] = useState(0);
  const [postsLoading, setPostsLoading] = useState(false);
  const [collectionsLoading, setCollectionsLoading] = useState(false);
  const [pendingPostId, setPendingPostId] = useState<number | null>(null);
  const [error, setError] = useState("");

  const pageCount = useMemo(() => Math.max(1, Math.ceil(total / PAGE_SIZE)), [total]);

  const loadPosts = async (nextPage = page) => {
    setPostsLoading(true);
    setError("");
    try {
      const data = await listMyPosts(nextPage, PAGE_SIZE);
      setPosts(data.items);
      setTotal(data.total);
      setPage(data.page);
    } catch (e) {
      setError(e instanceof Error ? e.message : "加载我的文章失败");
    } finally {
      setPostsLoading(false);
    }
  };

  const loadCollections = async () => {
    setCollectionsLoading(true);
    setError("");
    try {
      const data = await listMyCollections();
      setCollections(data.items);
    } catch (e) {
      setError(e instanceof Error ? e.message : "加载我的合集失败");
    } finally {
      setCollectionsLoading(false);
    }
  };

  useEffect(() => {
    void loadPosts(page);
  }, [page]);

  useEffect(() => {
    void loadCollections();
  }, []);

  const onDeletePost = async (postId: number) => {
    if (!window.confirm("确认删除这篇文章吗？")) {
      return;
    }
    setPendingPostId(postId);
    setError("");
    try {
      await deletePost(postId);
      if (posts.length === 1 && page > 1) {
        setPage((prev) => Math.max(1, prev - 1));
      } else {
        await loadPosts(page);
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : "删除文章失败");
    } finally {
      setPendingPostId(null);
    }
  };

  const onLogout = async () => {
    try {
      await logout();
    } finally {
      authStore.clear();
      navigate("/");
    }
  };

  if (!auth.user) {
    return null;
  }

  return (
    <div className="min-h-screen bg-background">
      <div className="border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
          <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-4">
            <div className="flex items-center gap-4">
              <Avatar className="w-14 h-14">
                <AvatarFallback className="bg-primary/10 text-primary text-lg">
                  {auth.user.username.charAt(0).toUpperCase()}
                </AvatarFallback>
              </Avatar>
              <div>
                <h1 className="text-2xl font-bold text-foreground">个人中心</h1>
                <p className="text-muted-foreground">{auth.user.username}</p>
                <p className="text-xs text-muted-foreground">注册时间：{formatAbsoluteDateTime(auth.user.createdAt)}</p>
              </div>
            </div>
            <Badge variant={auth.user.role === "admin" ? "default" : "secondary"}>
              {auth.user.role === "admin" ? "管理员" : "普通用户"}
            </Badge>
          </div>
          {error ? <p className="text-sm text-destructive mt-4">{error}</p> : null}
        </div>
      </div>

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6 space-y-6">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
          <Card>
            <CardHeader className="pb-2">
              <CardDescription>我的文章</CardDescription>
              <CardTitle className="text-2xl">{total}</CardTitle>
            </CardHeader>
          </Card>
          <Card>
            <CardHeader className="pb-2">
              <CardDescription>我的合集</CardDescription>
              <CardTitle className="text-2xl">{collections.length}</CardTitle>
            </CardHeader>
          </Card>
          <Card>
            <CardContent className="pt-6">
              <Button asChild className="w-full gap-2">
                <Link to="/editor/new">
                  <FilePenLine className="w-4 h-4" />
                  写新文章
                </Link>
              </Button>
            </CardContent>
          </Card>
          <Card>
            <CardContent className="pt-6 flex flex-col gap-2">
              <Button asChild variant="outline" className="w-full gap-2">
                <Link to="/account/password">
                  <KeyRound className="w-4 h-4" />
                  修改密码
                </Link>
              </Button>
              <Button variant="destructive" className="w-full gap-2" onClick={onLogout}>
                <LogOut className="w-4 h-4" />
                退出登录
              </Button>
            </CardContent>
          </Card>
        </div>

        <Tabs defaultValue="posts" className="w-full">
          <TabsList>
            <TabsTrigger value="posts" className="gap-2">
              <UserCircle2 className="w-4 h-4" />
              我的文章
            </TabsTrigger>
            <TabsTrigger value="collections" className="gap-2">
              <BookOpen className="w-4 h-4" />
              我的合集
            </TabsTrigger>
          </TabsList>

          <TabsContent value="posts">
            <Card>
              <CardHeader>
                <CardTitle>文章管理</CardTitle>
                <CardDescription>编辑、查看或删除你发布的文章。</CardDescription>
              </CardHeader>
              <CardContent className="space-y-4">
                <Table>
                  <TableHeader>
                    <TableRow>
                      <TableHead className="w-[80px]">ID</TableHead>
                      <TableHead>标题</TableHead>
                      <TableHead>发布时间</TableHead>
                      <TableHead>更新时间</TableHead>
                      <TableHead className="text-right">操作</TableHead>
                    </TableRow>
                  </TableHeader>
                  <TableBody>
                    {postsLoading ? (
                      <TableRow>
                        <TableCell colSpan={5} className="text-center text-muted-foreground py-8">
                          正在加载文章...
                        </TableCell>
                      </TableRow>
                    ) : posts.length === 0 ? (
                      <TableRow>
                        <TableCell colSpan={5} className="text-center text-muted-foreground py-8">
                          你还没有发布文章
                        </TableCell>
                      </TableRow>
                    ) : (
                      posts.map((post) => (
                        <TableRow key={post.id}>
                          <TableCell>{post.id}</TableCell>
                          <TableCell className="max-w-[420px]">
                            <div className="line-clamp-2 font-medium">{post.title}</div>
                          </TableCell>
                          <TableCell>{formatAbsoluteDateTime(post.createdAt)}</TableCell>
                          <TableCell>{formatAbsoluteDateTime(post.updatedAt)}</TableCell>
                          <TableCell>
                            <div className="flex justify-end gap-2">
                              <Button size="sm" variant="outline" asChild>
                                <Link to={`/posts/${post.id}`}>查看</Link>
                              </Button>
                              <Button size="sm" variant="outline" asChild>
                                <Link to={`/editor/${post.id}`}>编辑</Link>
                              </Button>
                              <Button
                                size="sm"
                                variant="destructive"
                                disabled={pendingPostId === post.id}
                                onClick={() => onDeletePost(post.id)}
                              >
                                <Trash2 className="w-4 h-4" />
                              </Button>
                            </div>
                          </TableCell>
                        </TableRow>
                      ))
                    )}
                  </TableBody>
                </Table>

                <div className="flex items-center justify-between">
                  <p className="text-sm text-muted-foreground">
                    第 {page} / {pageCount} 页，共 {total} 篇
                  </p>
                  <div className="flex gap-2">
                    <Button variant="outline" size="sm" disabled={page <= 1} onClick={() => setPage((p) => p - 1)}>
                      上一页
                    </Button>
                    <Button
                      variant="outline"
                      size="sm"
                      disabled={page >= pageCount}
                      onClick={() => setPage((p) => p + 1)}
                    >
                      下一页
                    </Button>
                  </div>
                </div>
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="collections">
            <Card>
              <CardHeader>
                <CardTitle>合集管理</CardTitle>
                <CardDescription>进入合集页面可创建合集、增删文章顺序。</CardDescription>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex justify-end">
                  <Button asChild>
                    <Link to="/collections">管理我的合集</Link>
                  </Button>
                </div>
                {collectionsLoading ? (
                  <p className="text-sm text-muted-foreground">正在加载合集...</p>
                ) : collections.length === 0 ? (
                  <p className="text-sm text-muted-foreground">你还没有合集，去创建第一个合集吧。</p>
                ) : (
                  <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                    {collections.map((collection) => (
                      <Card key={collection.id} className="card-hover">
                        <CardContent className="pt-6 space-y-2">
                          <h3 className="font-semibold line-clamp-1">{collection.name}</h3>
                          <p className="text-sm text-muted-foreground line-clamp-2">
                            {collection.description || "暂无描述"}
                          </p>
                          <p className="text-xs text-muted-foreground">
                            {collection.postCount} 篇 · 更新于 {formatAbsoluteDateTime(collection.updatedAt)}
                          </p>
                          <Button asChild size="sm" variant="outline" className="w-full">
                            <Link to={`/collections/${collection.id}`}>进入合集</Link>
                          </Button>
                        </CardContent>
                      </Card>
                    ))}
                  </div>
                )}
              </CardContent>
            </Card>
          </TabsContent>
        </Tabs>
      </div>
    </div>
  );
}
