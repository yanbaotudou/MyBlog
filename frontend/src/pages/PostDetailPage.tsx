import { useEffect, useMemo, useState } from "react";
import ReactMarkdown from "react-markdown";
import { Link, useNavigate, useParams, useSearchParams } from "react-router-dom";
import { addPostToCollection, getPostCollections, listMyCollections } from "../api/collections";
import { deletePost, getPost } from "../api/posts";
import { useAuthState } from "../store/authStore";
import type { Collection, CollectionNavigation, PostCollectionMembership } from "../types/collection";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";

export function PostDetailPage() {
  const params = useParams();
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const auth = useAuthState();

  const [post, setPost] = useState<Post | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [deleting, setDeleting] = useState(false);

  const [memberships, setMemberships] = useState<PostCollectionMembership[]>([]);
  const [navigation, setNavigation] = useState<CollectionNavigation | null>(null);
  const [collectionsLoading, setCollectionsLoading] = useState(false);
  const [collectionsError, setCollectionsError] = useState("");

  const [myCollections, setMyCollections] = useState<Collection[]>([]);
  const [selectedAddCollectionId, setSelectedAddCollectionId] = useState<number>(0);
  const [addingToCollection, setAddingToCollection] = useState(false);

  const selectedCollectionId = (() => {
    const raw = searchParams.get("collectionId");
    if (!raw) {
      return undefined;
    }
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
    if (!post) {
      return;
    }

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
      setSelectedAddCollectionId(0);
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
    if (!post || !auth.user) {
      return false;
    }
    return auth.user.role === "admin" || auth.user.id === post.authorId;
  }, [auth.user, post]);

  const handleDelete = async () => {
    if (!post) {
      return;
    }
    if (!window.confirm("确认删除这篇文章吗？")) {
      return;
    }
    setDeleting(true);
    try {
      await deletePost(post.id);
      navigate("/");
    } catch (e) {
      setError(e instanceof Error ? e.message : "删除失败");
      setDeleting(false);
    }
  };

  const handleCollectionContextChange = (collectionIdValue: string) => {
    if (!post) {
      return;
    }
    const id = Number(collectionIdValue);
    if (!id) {
      navigate(`/posts/${post.id}`);
      return;
    }
    navigate(`/posts/${post.id}?collectionId=${id}`);
  };

  const handleAddCurrentPostToCollection = async () => {
    if (!post || !selectedAddCollectionId) {
      return;
    }

    setAddingToCollection(true);
    setCollectionsError("");
    try {
      await addPostToCollection(selectedAddCollectionId, post.id);
      const data = await getPostCollections(post.id, selectedCollectionId);
      setMemberships(data.items);
      setNavigation(data.navigation);
    } catch (e) {
      setCollectionsError(e instanceof Error ? e.message : "加入合集失败");
    } finally {
      setAddingToCollection(false);
    }
  };

  if (loading) {
    return <main className="container status-line">正在加载文章...</main>;
  }

  if (error) {
    return <main className="container status-line error">{error}</main>;
  }

  if (!post) {
    return <main className="container empty-state">文章不存在。</main>;
  }

  return (
    <main className="container detail-page">
      <header className="detail-header">
        <h1>{post.title}</h1>
        <div className="post-meta">
          <span className="meta-pill">作者：{post.authorUsername}</span>
          <time className="meta-pill" dateTime={post.createdAt} title={formatAbsoluteDateTime(post.createdAt)}>
            发布时间：{formatAbsoluteDateTime(post.createdAt)}
          </time>
          <time className="meta-pill" dateTime={post.updatedAt} title={formatAbsoluteDateTime(post.updatedAt)}>
            更新时间：{formatAbsoluteDateTime(post.updatedAt)}
          </time>
        </div>

        <section className="collection-context-panel">
          <div className="collection-context-row">
            <label htmlFor="collection-context-select">合集上下文</label>
            <select
              id="collection-context-select"
              value={selectedCollectionId ?? ""}
              onChange={(e) => handleCollectionContextChange(e.target.value)}
              disabled={collectionsLoading}
            >
              <option value="">不使用合集导航</option>
              {memberships.map((item) => (
                <option key={item.collectionId} value={item.collectionId}>
                  {item.collectionName}（第 {item.position} 篇）
                </option>
              ))}
            </select>
            {selectedCollectionId ? (
              <Link className="secondary-btn" to={`/collections/${selectedCollectionId}`}>
                查看合集
              </Link>
            ) : null}
          </div>

          {selectedCollectionId && navigation ? (
            <div className="collection-nav-row">
              {navigation.prev ? (
                <Link className="secondary-btn" to={`/posts/${navigation.prev.id}?collectionId=${selectedCollectionId}`}>
                  上一篇：{navigation.prev.title}
                </Link>
              ) : (
                <button className="secondary-btn" disabled>
                  已是第一篇
                </button>
              )}

              {navigation.next ? (
                <Link className="secondary-btn" to={`/posts/${navigation.next.id}?collectionId=${selectedCollectionId}`}>
                  下一篇：{navigation.next.title}
                </Link>
              ) : (
                <button className="secondary-btn" disabled>
                  已是最后一篇
                </button>
              )}
            </div>
          ) : null}

          {collectionsError ? <p className="status-line error">{collectionsError}</p> : null}
        </section>

        {auth.user && myCollections.length > 0 ? (
          <section className="collection-context-row add-to-collection-row">
            <label htmlFor="add-collection-select">加入我的合集</label>
            <select
              id="add-collection-select"
              value={selectedAddCollectionId || ""}
              onChange={(e) => setSelectedAddCollectionId(Number(e.target.value))}
              disabled={addingToCollection}
            >
              {myCollections.map((item) => (
                <option key={item.id} value={item.id}>
                  {item.name}
                </option>
              ))}
            </select>
            <button className="primary-btn" onClick={handleAddCurrentPostToCollection} disabled={addingToCollection}>
              {addingToCollection ? "加入中..." : "加入合集"}
            </button>
          </section>
        ) : null}

        {canEdit ? (
          <div className="action-row">
            <Link className="secondary-btn" to={`/editor/${post.id}`}>
              编辑
            </Link>
            <button className="danger-btn" onClick={handleDelete} disabled={deleting}>
              删除
            </button>
          </div>
        ) : null}
      </header>

      <article className="markdown-body">
        <ReactMarkdown>{post.contentMarkdown}</ReactMarkdown>
      </article>
    </main>
  );
}
