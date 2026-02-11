import { Link } from "react-router-dom";
import type { Post } from "../types/post";
import { formatAbsoluteDateTime } from "../utils/dateTime";

interface PostCardProps {
  post: Post;
}

function excerpt(input: string): string {
  const stripped = input.replace(/[#>*`\-\[\]()]/g, " ").replace(/\s+/g, " ").trim();
  if (stripped.length <= 180) {
    return stripped;
  }
  return `${stripped.slice(0, 180)}...`;
}

export function PostCard({ post }: PostCardProps) {
  return (
    <article className="post-card">
      <h3>
        <Link to={`/posts/${post.id}`}>{post.title}</Link>
      </h3>
      <p>{excerpt(post.contentMarkdown)}</p>
      <div className="post-meta">
        <span className="meta-pill">作者：{post.authorUsername}</span>
        <time className="meta-pill" dateTime={post.updatedAt} title={formatAbsoluteDateTime(post.updatedAt)}>
          更新时间：{formatAbsoluteDateTime(post.updatedAt)}
        </time>
        <time className="meta-pill" dateTime={post.createdAt} title={formatAbsoluteDateTime(post.createdAt)}>
          发布时间：{formatAbsoluteDateTime(post.createdAt)}
        </time>
      </div>
    </article>
  );
}
