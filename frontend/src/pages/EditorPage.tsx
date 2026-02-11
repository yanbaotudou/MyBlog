import { useEffect, useMemo, useState } from "react";
import ReactMarkdown from "react-markdown";
import {
  ArrowLeft,
  Bold,
  Code,
  Eye,
  EyeOff,
  Heading,
  Italic,
  Link as LinkIcon,
  List,
  ListOrdered,
  Maximize2,
  Minimize2,
  Quote,
  Save,
  Send
} from "lucide-react";
import { Link, useNavigate, useParams } from "react-router-dom";
import { createPost, getPost, updatePost } from "../api/posts";
import { useAuthState } from "../store/authStore";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Separator } from "@/components/ui/separator";

export function EditorPage() {
  const params = useParams();
  const navigate = useNavigate();
  const auth = useAuthState();

  const isEdit = useMemo(() => Boolean(params.id), [params.id]);
  const [title, setTitle] = useState("");
  const [content, setContent] = useState("");
  const [ownerId, setOwnerId] = useState<number | null>(null);
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [showPreview, setShowPreview] = useState(true);
  const [isFullscreen, setIsFullscreen] = useState(false);
  const [lastSavedAt, setLastSavedAt] = useState<string>("");
  const [error, setError] = useState("");

  useEffect(() => {
    if (!isEdit || !params.id) {
      return;
    }

    const id = Number(params.id);
    if (!id) {
      setError("文章 ID 无效");
      return;
    }

    const run = async () => {
      setLoading(true);
      setError("");
      try {
        const post = await getPost(id);
        setTitle(post.title);
        setContent(post.contentMarkdown);
        setOwnerId(post.authorId);
      } catch (e) {
        setError(e instanceof Error ? e.message : "加载失败");
      } finally {
        setLoading(false);
      }
    };

    void run();
  }, [isEdit, params.id]);

  const canEdit = useMemo(() => {
    if (!isEdit) {
      return true;
    }
    if (!auth.user || ownerId === null) {
      return false;
    }
    return auth.user.role === "admin" || auth.user.id === ownerId;
  }, [auth.user, isEdit, ownerId]);

  const onSave = async () => {
    if (!title.trim() || !content.trim()) {
      setError("标题和正文不能为空");
      return;
    }

    setSaving(true);
    setError("");
    try {
      if (isEdit && params.id) {
        const updated = await updatePost(Number(params.id), {
          title: title.trim(),
          contentMarkdown: content
        });
        setLastSavedAt(new Date().toLocaleTimeString("zh-CN", { hour12: false }));
        navigate(`/posts/${updated.id}`);
      } else {
        const created = await createPost({
          title: title.trim(),
          contentMarkdown: content
        });
        setLastSavedAt(new Date().toLocaleTimeString("zh-CN", { hour12: false }));
        navigate(`/posts/${created.id}`);
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : "保存失败");
    } finally {
      setSaving(false);
    }
  };

  const insertMarkdown = (before: string, after = "") => {
    const textarea = document.getElementById("editor-content") as HTMLTextAreaElement | null;
    if (!textarea) {
      return;
    }

    const start = textarea.selectionStart;
    const end = textarea.selectionEnd;
    const selected = content.slice(start, end);
    const next = `${content.slice(0, start)}${before}${selected}${after}${content.slice(end)}`;
    setContent(next);

    setTimeout(() => {
      textarea.focus();
      const cursorStart = start + before.length;
      const cursorEnd = end + before.length;
      textarea.setSelectionRange(cursorStart, cursorEnd);
    }, 0);
  };

  const toolbarItems: Array<{
    icon: React.ComponentType<{ className?: string }>;
    title: string;
    action: () => void;
  }> = [
    { icon: Bold, title: "粗体", action: () => insertMarkdown("**", "**") },
    { icon: Italic, title: "斜体", action: () => insertMarkdown("*", "*") },
    { icon: Heading, title: "标题", action: () => insertMarkdown("## ") },
    { icon: LinkIcon, title: "链接", action: () => insertMarkdown("[", "](url)") },
    { icon: Code, title: "行内代码", action: () => insertMarkdown("`", "`") },
    { icon: List, title: "无序列表", action: () => insertMarkdown("- ") },
    { icon: ListOrdered, title: "有序列表", action: () => insertMarkdown("1. ") },
    { icon: Quote, title: "引用", action: () => insertMarkdown("> ") }
  ];

  const headerTitle = isEdit ? "编辑文章" : "新建文章";
  const headerDescription = isEdit ? "修改并保存这篇文章" : "记录你的学习过程并发布";
  const saveLabel = isEdit ? "更新文章" : "发布文章";

  if (loading) {
    return <div className="min-h-screen flex items-center justify-center text-muted-foreground">正在加载编辑内容...</div>;
  }

  if (isEdit && !canEdit) {
    return (
      <div className="min-h-screen flex items-center justify-center px-4">
        <Card className="max-w-md w-full">
          <CardHeader>
            <CardTitle>无权限</CardTitle>
            <CardDescription>你不是这篇文章的作者，也不是管理员。</CardDescription>
          </CardHeader>
          <CardContent>
            <Button asChild className="w-full">
              <Link to="/">返回首页</Link>
            </Button>
          </CardContent>
        </Card>
      </div>
    );
  }

  return (
    <div className={`min-h-screen bg-background ${isFullscreen ? "fixed inset-0 z-50" : ""}`}>
      <header className="sticky top-0 z-40 bg-background/90 backdrop-blur-md border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="h-16 flex items-center justify-between gap-3">
            <div className="flex items-center gap-3 min-w-0 flex-1">
              <Button variant="ghost" size="sm" className="gap-2 shrink-0" onClick={() => navigate(-1)}>
                <ArrowLeft className="w-4 h-4" />
                返回
              </Button>
              <Input
                id="editor-title"
                value={title}
                onChange={(e) => setTitle(e.target.value)}
                placeholder="请输入文章标题（必填）..."
                maxLength={120}
                className="border-0 bg-transparent text-base sm:text-lg font-medium shadow-none focus-visible:ring-0 px-0"
              />
            </div>

            <div className="flex items-center gap-2 shrink-0">
              <Badge variant="secondary" className="hidden md:inline-flex">
                {isEdit ? "编辑模式" : "创建模式"}
              </Badge>
              <span className="hidden lg:inline text-xs text-muted-foreground w-[90px] text-right">
                {saving ? "保存中..." : lastSavedAt ? `已保存 ${lastSavedAt}` : ""}
              </span>
              <Button variant="outline" size="sm" className="gap-2" onClick={onSave} disabled={saving}>
                <Save className="w-4 h-4" />
                保存
              </Button>
              <Button size="sm" className="gap-2 btn-hover-lift" onClick={onSave} disabled={saving}>
                <Send className="w-4 h-4" />
                {saving ? "处理中..." : saveLabel}
              </Button>
              <Button
                variant="ghost"
                size="icon"
                aria-label="切换全屏"
                onClick={() => setIsFullscreen((prev) => !prev)}
              >
                {isFullscreen ? <Minimize2 className="w-4 h-4" /> : <Maximize2 className="w-4 h-4" />}
              </Button>
            </div>
          </div>
        </div>
      </header>

      <div className="border-b border-border bg-secondary/30">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="py-2 flex items-center gap-1 flex-wrap">
            {toolbarItems.map((item) => {
              const Icon = item.icon;
              return (
                <Button
                  key={item.title}
                  variant="ghost"
                  size="icon"
                  className="h-8 w-8"
                  title={item.title}
                  aria-label={item.title}
                  onClick={item.action}
                >
                  <Icon className="w-4 h-4" />
                </Button>
              );
            })}
            <Separator orientation="vertical" className="h-6 mx-2" />
            <Button variant="ghost" size="sm" className="gap-2" onClick={() => setShowPreview((prev) => !prev)}>
              {showPreview ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
              {showPreview ? "隐藏预览" : "显示预览"}
            </Button>
            <div className="ml-auto flex items-center gap-3 text-xs text-muted-foreground">
              <span>{title.length} / 120</span>
              <span>{content.length} / 50000</span>
            </div>
          </div>
        </div>
      </div>

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6">
        <div className="mb-4 flex items-center justify-between gap-3 flex-wrap">
          <div>
            <h1 className="text-2xl font-bold text-foreground">{headerTitle}</h1>
            <p className="text-sm text-muted-foreground">{headerDescription}</p>
          </div>
          <Badge variant="secondary">{isEdit ? "编辑模式" : "创建模式"}</Badge>
        </div>

        {error ? <p className="text-sm text-destructive mb-4">{error}</p> : null}

        <div className={`grid gap-6 ${showPreview ? "grid-cols-1 lg:grid-cols-2" : "grid-cols-1"}`}>
          <Card className="border-border/70">
            <CardHeader className="pb-3">
              <CardTitle>Markdown 编辑</CardTitle>
              <CardDescription>支持快捷工具栏，正文最大 50000 字。</CardDescription>
            </CardHeader>
            <CardContent className="pt-0">
              <textarea
                id="editor-content"
                value={content}
                onChange={(e) => setContent(e.target.value)}
                placeholder="开始写作..."
                maxLength={50000}
                className="min-h-[60vh] w-full resize-y rounded-md border border-input bg-transparent px-3 py-3 text-sm font-mono outline-none focus-visible:ring-2 focus-visible:ring-ring"
              />
            </CardContent>
          </Card>

          {showPreview ? (
            <Card className="border-border/70">
              <CardHeader className="pb-3">
                <CardTitle className="flex items-center gap-2">
                  <Eye className="w-4 h-4" />
                  实时预览
                </CardTitle>
                <CardDescription>最终展示效果预览。</CardDescription>
              </CardHeader>
              <CardContent className="pt-0">
                <div className="min-h-[60vh] rounded-md border border-border/60 bg-card p-6 overflow-auto">
                  <h1 className="text-3xl font-bold mb-4">{title.trim() || "未命名标题"}</h1>
                  <div className="markdown-content">
                    <ReactMarkdown>{content || "_暂无内容_"}</ReactMarkdown>
                  </div>
                </div>
              </CardContent>
            </Card>
          ) : null}
        </div>
      </div>
    </div>
  );
}
