import { FormEvent, useEffect, useState } from "react";
import { ExternalLink, FileText, Folder, MoreVertical, Plus, Trash2 } from "lucide-react";
import { useNavigate } from "react-router-dom";
import { createCollection, listMyCollections } from "../api/collections";
import type { Collection } from "../types/collection";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
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
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Textarea } from "@/components/ui/textarea";

export function CollectionsPage() {
  const navigate = useNavigate();
  const [collections, setCollections] = useState<Collection[]>([]);
  const [showCreateDialog, setShowCreateDialog] = useState(false);
  const [newCollectionName, setNewCollectionName] = useState("");
  const [newCollectionDesc, setNewCollectionDesc] = useState("");
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

  const handleCreateCollection = async (e?: FormEvent) => {
    e?.preventDefault();
    if (!newCollectionName.trim()) {
      setError("请输入合集名称");
      return;
    }
    setSubmitting(true);
    setError("");
    try {
      await createCollection({
        name: newCollectionName.trim(),
        description: newCollectionDesc.trim()
      });
      setNewCollectionName("");
      setNewCollectionDesc("");
      setShowCreateDialog(false);
      await loadCollections();
    } catch (e) {
      setError(e instanceof Error ? e.message : "创建合集失败");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="min-h-screen bg-background">
      <div className="border-b border-border">
        <div className="max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-2xl font-bold text-foreground">我的合集</h1>
              <p className="text-muted-foreground mt-1">管理和组织你的文章</p>
            </div>

            <Dialog open={showCreateDialog} onOpenChange={setShowCreateDialog}>
              <DialogTrigger asChild>
                <Button className="gap-2 btn-hover-lift">
                  <Plus className="w-4 h-4" />
                  新建合集
                </Button>
              </DialogTrigger>
              <DialogContent>
                <DialogHeader>
                  <DialogTitle>创建新合集</DialogTitle>
                  <DialogDescription>创建一个新的文章合集来组织你的内容</DialogDescription>
                </DialogHeader>
                <form className="space-y-4 py-4" onSubmit={handleCreateCollection}>
                  <div className="space-y-2">
                    <Label htmlFor="collection-name">合集名称</Label>
                    <Input
                      id="collection-name"
                      placeholder="例如：React进阶系列"
                      value={newCollectionName}
                      onChange={(e) => setNewCollectionName(e.target.value)}
                      maxLength={80}
                    />
                  </div>
                  <div className="space-y-2">
                    <Label htmlFor="collection-description">描述（可选）</Label>
                    <Textarea
                      id="collection-description"
                      placeholder="简要描述这个合集的内容..."
                      value={newCollectionDesc}
                      onChange={(e) => setNewCollectionDesc(e.target.value)}
                      maxLength={500}
                    />
                  </div>
                  <DialogFooter>
                    <Button type="button" variant="outline" onClick={() => setShowCreateDialog(false)}>
                      取消
                    </Button>
                    <Button type="submit" disabled={submitting}>
                      {submitting ? "创建中..." : "创建"}
                    </Button>
                  </DialogFooter>
                </form>
              </DialogContent>
            </Dialog>
          </div>
          {error ? <p className="text-sm text-destructive mt-3">{error}</p> : null}
        </div>
      </div>

      <div className="max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {loading ? <p className="text-sm text-muted-foreground">加载中...</p> : null}

        {collections.length > 0 ? (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {collections.map((collection) => (
              <Card
                key={collection.id}
                className="card-hover cursor-pointer group"
                onClick={() => navigate(`/collections/${collection.id}`)}
              >
                <CardContent className="p-6">
                  <div className="flex items-start justify-between mb-4">
                    <div className="w-12 h-12 bg-primary/10 rounded-xl flex items-center justify-center">
                      <Folder className="w-6 h-6 text-primary" />
                    </div>
                    <DropdownMenu>
                      <DropdownMenuTrigger asChild>
                        <Button
                          variant="ghost"
                          size="icon"
                          className="h-8 w-8 opacity-0 group-hover:opacity-100 transition-opacity"
                          onClick={(e) => e.stopPropagation()}
                        >
                          <MoreVertical className="w-4 h-4" />
                        </Button>
                      </DropdownMenuTrigger>
                      <DropdownMenuContent align="end">
                        <DropdownMenuItem
                          onClick={(e) => {
                            e.stopPropagation();
                            navigate(`/collections/${collection.id}`);
                          }}
                        >
                          <ExternalLink className="mr-2 h-4 w-4" />
                          查看详情
                        </DropdownMenuItem>
                        <DropdownMenuSeparator />
                        <DropdownMenuItem
                          className="text-destructive"
                          onClick={(e) => {
                            e.stopPropagation();
                            setError("当前版本暂不支持删除合集");
                          }}
                        >
                          <Trash2 className="mr-2 h-4 w-4" />
                          删除
                        </DropdownMenuItem>
                      </DropdownMenuContent>
                    </DropdownMenu>
                  </div>

                  <h3 className="text-lg font-semibold text-foreground mb-2 line-clamp-1">{collection.name}</h3>
                  <p className="text-muted-foreground text-sm mb-4 line-clamp-2">{collection.description || "暂无描述"}</p>

                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-4 text-sm text-muted-foreground">
                      <span className="flex items-center gap-1">
                        <FileText className="w-4 h-4" />
                        {collection.postCount} 篇
                      </span>
                      <span>{formatAbsoluteDateTime(collection.updatedAt)}</span>
                    </div>
                    <Badge variant="secondary">公开</Badge>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        ) : !loading ? (
          <div className="text-center py-16">
            <div className="w-16 h-16 bg-muted rounded-full flex items-center justify-center mx-auto mb-4">
              <Folder className="w-8 h-8 text-muted-foreground" />
            </div>
            <h3 className="text-lg font-medium text-foreground mb-2">暂无合集</h3>
            <p className="text-muted-foreground mb-4">创建一个合集来组织你的文章</p>
            <Button onClick={() => setShowCreateDialog(true)}>
              <Plus className="w-4 h-4 mr-2" />
              新建合集
            </Button>
          </div>
        ) : null}
      </div>
    </div>
  );
}
