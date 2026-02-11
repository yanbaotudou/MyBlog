import { useEffect, useMemo, useState } from "react";
import { useNavigate, useParams } from "react-router-dom";
import { createPost, getPost, updatePost } from "../api/posts";
import { MarkdownEditor } from "../components/MarkdownEditor";
import { useAuthState } from "../store/authStore";

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
        navigate(`/posts/${updated.id}`);
      } else {
        const created = await createPost({
          title: title.trim(),
          contentMarkdown: content
        });
        navigate(`/posts/${created.id}`);
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : "保存失败");
      setSaving(false);
    }
  };

  if (loading) {
    return <main className="container status-line">正在加载编辑内容...</main>;
  }

  if (isEdit && !canEdit) {
    return <main className="container status-line error">你没有权限编辑这篇文章。</main>;
  }

  return (
    <main className="container editor-page">
      <h1>{isEdit ? "编辑文章" : "新建文章"}</h1>
      <MarkdownEditor title={title} setTitle={setTitle} content={content} setContent={setContent} />

      {error ? <p className="status-line error">{error}</p> : null}

      <div className="action-row">
        <button className="primary-btn" onClick={onSave} disabled={saving}>
          {saving ? "保存中..." : "保存文章"}
        </button>
      </div>
    </main>
  );
}
