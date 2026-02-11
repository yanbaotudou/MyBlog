import { FormEvent, useEffect, useMemo, useState } from "react";
import { Helmet } from "react-helmet-async";
import { Link, useParams } from "react-router-dom";
import { addPostToCollection, getCollection, removePostFromCollection } from "../api/collections";
import { listPosts } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { CollectionDetail } from "../types/collection";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";

function stripForMeta(input: string): string {
  return input.replace(/\s+/g, " ").trim();
}

function clampText(input: string, max: number): string {
  if (input.length <= max) {
    return input;
  }
  return `${input.slice(0, max - 1)}...`;
}

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

function buildAbsoluteUrl(siteUrl: string, path: string): string {
  if (!siteUrl) {
    return path;
  }
  return `${siteUrl}${path.startsWith("/") ? path : `/${path}`}`;
}

async function writeToClipboard(text: string): Promise<boolean> {
  try {
    if (typeof navigator !== "undefined" && navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
      return true;
    }
  } catch {
    // noop: fallback below
  }

  if (typeof document === "undefined") {
    return false;
  }

  const textarea = document.createElement("textarea");
  textarea.value = text;
  textarea.style.position = "fixed";
  textarea.style.left = "-9999px";
  document.body.appendChild(textarea);
  textarea.focus();
  textarea.select();
  const ok = document.execCommand("copy");
  document.body.removeChild(textarea);
  return ok;
}

export function CollectionDetailPage() {
  const params = useParams();
  const auth = useAuthState();
  const [detail, setDetail] = useState<CollectionDetail | null>(null);
  const [allPosts, setAllPosts] = useState<Post[]>([]);
  const [selectedPostId, setSelectedPostId] = useState<number>(0);
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const [shareMessage, setShareMessage] = useState("");
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

      const isEditor =
        !!auth.user &&
        (auth.user.role === "admin" || auth.user.id === collectionData.collection.ownerId);

      if (isEditor) {
        const postsData = await listPosts(1, 100);
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

  useEffect(() => {
    if (!shareMessage || typeof window === "undefined") {
      return;
    }
    const timer = window.setTimeout(() => setShareMessage(""), 2600);
    return () => window.clearTimeout(timer);
  }, [shareMessage]);

  const canEdit = useMemo(() => {
    if (!detail || !auth.user) {
      return false;
    }
    return auth.user.role === "admin" || auth.user.id === detail.collection.ownerId;
  }, [auth.user, detail]);

  const availablePosts = useMemo(() => {
    if (!detail) {
      return [];
    }
    const existingIds = new Set(detail.posts.map((post) => post.id));
    return allPosts.filter((post) => !existingIds.has(post.id));
  }, [allPosts, detail]);

  const canonicalUrl = useMemo(() => {
    if (!collectionId) {
      return siteUrl || "";
    }
    return buildAbsoluteUrl(siteUrl, `/collections/${collectionId}`);
  }, [siteUrl, collectionId]);

  const ogImageUrl = useMemo(() => buildAbsoluteUrl(siteUrl, "/og-cover.svg"), [siteUrl]);

  const metaTitle = useMemo(() => {
    if (!detail) {
      return "学习合集 | 学习日志";
    }
    return `${detail.collection.name} | 学习合集`;
  }, [detail]);

  const metaDescription = useMemo(() => {
    if (!detail) {
      return "学习日志博客合集页面，持续整理学习路径。";
    }
    const fallback = `${detail.collection.ownerUsername} 创建的学习合集，共 ${detail.total} 篇文章。`;
    return clampText(stripForMeta(detail.collection.description || fallback), 160);
  }, [detail]);

  const metaKeywords = useMemo(() => {
    if (!detail) {
      return "学习日志,学习合集,博客";
    }
    const postKeywords = detail.posts.slice(0, 5).map((post) => post.title.trim()).filter(Boolean);
    return ["学习日志", "学习合集", detail.collection.name, ...postKeywords].join(",");
  }, [detail]);

  const structuredData = useMemo(() => {
    if (!detail) {
      return "";
    }
    const itemList = detail.posts.map((post, index) => ({
      "@type": "ListItem",
      position: index + 1,
      name: post.title,
      url: buildAbsoluteUrl(siteUrl, `/posts/${post.id}?collectionId=${detail.collection.id}`)
    }));
    return JSON.stringify({
      "@context": "https://schema.org",
      "@type": "CollectionPage",
      name: detail.collection.name,
      description: metaDescription,
      url: canonicalUrl,
      dateModified: detail.collection.updatedAt,
      creator: {
        "@type": "Person",
        name: detail.collection.ownerUsername
      },
      mainEntity: {
        "@type": "ItemList",
        itemListElement: itemList
      }
    });
  }, [detail, siteUrl, canonicalUrl, metaDescription]);

  const shareUrl = canonicalUrl;
  const shareTitle = metaTitle;
  const shareText = `${metaDescription}（共 ${detail?.total ?? 0} 篇）`;
  const supportsNativeShare =
    typeof navigator !== "undefined" && typeof navigator.share === "function";

  const weiboShareUrl = useMemo(() => {
    if (!shareUrl) {
      return "";
    }
    return `https://service.weibo.com/share/share.php?title=${encodeURIComponent(shareTitle)}&url=${encodeURIComponent(
      shareUrl
    )}`;
  }, [shareTitle, shareUrl]);

  const xShareUrl = useMemo(() => {
    if (!shareUrl) {
      return "";
    }
    return `https://x.com/intent/tweet?text=${encodeURIComponent(shareTitle)}&url=${encodeURIComponent(shareUrl)}`;
  }, [shareTitle, shareUrl]);

  useEffect(() => {
    if (availablePosts.length > 0) {
      setSelectedPostId(availablePosts[0].id);
    } else {
      setSelectedPostId(0);
    }
  }, [availablePosts]);

  const onAddPost = async (e: FormEvent) => {
    e.preventDefault();
    if (!detail || !selectedPostId) {
      return;
    }

    setSaving(true);
    setError("");
    try {
      await addPostToCollection(detail.collection.id, selectedPostId);
      await loadData();
    } catch (e) {
      setError(e instanceof Error ? e.message : "添加文章失败");
    } finally {
      setSaving(false);
    }
  };

  const onRemovePost = async (postId: number) => {
    if (!detail) {
      return;
    }
    if (!window.confirm("确认从合集中移除该文章吗？")) {
      return;
    }

    setSaving(true);
    setError("");
    try {
      await removePostFromCollection(detail.collection.id, postId);
      await loadData();
    } catch (e) {
      setError(e instanceof Error ? e.message : "移除文章失败");
    } finally {
      setSaving(false);
    }
  };

  const onCopyLink = async () => {
    if (!shareUrl) {
      setShareMessage("当前链接不可用");
      return;
    }
    const ok = await writeToClipboard(shareUrl);
    setShareMessage(ok ? "合集链接已复制" : "复制失败，请手动复制地址栏");
  };

  const onNativeShare = async () => {
    if (!supportsNativeShare || !shareUrl || typeof navigator === "undefined") {
      setShareMessage("当前环境不支持系统分享");
      return;
    }
    try {
      await navigator.share({
        title: shareTitle,
        text: shareText,
        url: shareUrl
      });
    } catch {
      setShareMessage("已取消分享");
    }
  };

  const onCopyCatalog = async () => {
    if (!detail) {
      return;
    }
    const lines: string[] = [];
    lines.push(`${detail.collection.name}（学习合集）`);
    lines.push(shareUrl);
    lines.push("");
    lines.push("目录：");
    detail.posts.forEach((post, index) => {
      const url = buildAbsoluteUrl(siteUrl, `/posts/${post.id}?collectionId=${detail.collection.id}`);
      lines.push(`${index + 1}. ${post.title}`);
      lines.push(url);
    });
    const ok = await writeToClipboard(lines.join("\n"));
    setShareMessage(ok ? "合集目录已复制" : "复制失败，请稍后重试");
  };

  if (loading) {
    return (
      <>
        <Helmet>
          <title>加载合集中 | 学习日志</title>
        </Helmet>
        <main className="container status-line">正在加载合集...</main>
      </>
    );
  }

  if (error && !detail) {
    return (
      <>
        <Helmet>
          <title>合集加载失败 | 学习日志</title>
        </Helmet>
        <main className="container status-line error">{error}</main>
      </>
    );
  }

  if (!detail) {
    return (
      <>
        <Helmet>
          <title>合集不存在 | 学习日志</title>
          <meta name="robots" content="noindex" />
        </Helmet>
        <main className="container empty-state">合集不存在。</main>
      </>
    );
  }

  return (
    <>
      <Helmet>
        <title>{metaTitle}</title>
        <meta name="description" content={metaDescription} />
        <meta name="keywords" content={metaKeywords} />
        <meta property="og:type" content="website" />
        <meta property="og:site_name" content="学习日志博客" />
        <meta property="og:title" content={metaTitle} />
        <meta property="og:description" content={metaDescription} />
        <meta property="og:url" content={canonicalUrl} />
        <meta property="og:image" content={ogImageUrl} />
        <meta name="twitter:card" content="summary_large_image" />
        <meta name="twitter:title" content={metaTitle} />
        <meta name="twitter:description" content={metaDescription} />
        <meta name="twitter:image" content={ogImageUrl} />
        <link rel="canonical" href={canonicalUrl} />
        <script type="application/ld+json">{structuredData}</script>
      </Helmet>

      <main className="container collection-detail-page">
        <section className="collection-detail-head">
          <h1>{detail.collection.name}</h1>
          <p>{detail.collection.description || "暂无简介"}</p>
          <div className="post-meta">
            <span className="meta-pill">作者：{detail.collection.ownerUsername}</span>
            <span className="meta-pill">文章数：{detail.total}</span>
            <span className="meta-pill">更新：{formatAbsoluteDateTime(detail.collection.updatedAt)}</span>
          </div>
        </section>

        <section className="collection-share-card">
          <h2>分享合集</h2>
          <p>把这个学习路径分享给同伴，支持直接复制链接、系统分享和社交平台分享。</p>
          <div className="collection-share-actions">
            <button type="button" className="secondary-btn" onClick={onCopyLink}>
              复制链接
            </button>
            <button type="button" className="secondary-btn" onClick={onCopyCatalog}>
              复制目录
            </button>
            {supportsNativeShare ? (
              <button type="button" className="secondary-btn" onClick={onNativeShare}>
                系统分享
              </button>
            ) : null}
            {weiboShareUrl ? (
              <a className="secondary-btn" href={weiboShareUrl} target="_blank" rel="noreferrer">
                分享到微博
              </a>
            ) : null}
            {xShareUrl ? (
              <a className="secondary-btn" href={xShareUrl} target="_blank" rel="noreferrer">
                分享到 X
              </a>
            ) : null}
          </div>
          {shareMessage ? <p className="status-line">{shareMessage}</p> : null}
        </section>

        {canEdit ? (
          <section className="collection-add-post-card">
            <h2>添加文章到合集</h2>
            <form onSubmit={onAddPost} className="collection-add-form">
              <select
                value={selectedPostId || ""}
                onChange={(e) => setSelectedPostId(Number(e.target.value))}
                disabled={saving || availablePosts.length === 0}
              >
                {availablePosts.length === 0 ? <option value="">暂无可添加文章</option> : null}
                {availablePosts.map((post) => (
                  <option key={post.id} value={post.id}>
                    #{post.id} {post.title}
                  </option>
                ))}
              </select>
              <button type="submit" className="primary-btn" disabled={saving || !selectedPostId}>
                {saving ? "处理中..." : "加入合集"}
              </button>
            </form>
            {error ? <p className="status-line error">{error}</p> : null}
          </section>
        ) : null}

        <section className="collection-post-list">
          <h2>合集文章顺序</h2>
          {detail.posts.length === 0 ? <p className="empty-state">这个合集还没有文章。</p> : null}

          {detail.posts.map((post) => (
            <article key={post.id} className="collection-post-item">
              <div className="collection-post-main">
                <span className="meta-pill">第 {post.collectionPosition ?? "-"} 篇</span>
                <Link to={`/posts/${post.id}?collectionId=${detail.collection.id}`}>{post.title}</Link>
              </div>
              <div className="collection-post-actions">
                <span className="meta-pill">更新时间：{formatAbsoluteDateTime(post.updatedAt)}</span>
                {canEdit ? (
                  <button className="danger-btn" onClick={() => onRemovePost(post.id)} disabled={saving}>
                    移除
                  </button>
                ) : null}
              </div>
            </article>
          ))}
        </section>
      </main>
    </>
  );
}
