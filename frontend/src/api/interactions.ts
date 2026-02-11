import { apiRequest } from "./client";
import type { CommentItem, InteractionSummary, PagedComments } from "../types/interaction";
import type { PagedPosts } from "../types/post";

export function getPostInteractions(postId: number): Promise<InteractionSummary> {
  return apiRequest<InteractionSummary>(`/api/posts/${postId}/interactions`, {
    method: "GET"
  });
}

export function likePost(postId: number): Promise<InteractionSummary> {
  return apiRequest<InteractionSummary>(`/api/posts/${postId}/like`, {
    method: "PUT"
  });
}

export function unlikePost(postId: number): Promise<InteractionSummary> {
  return apiRequest<InteractionSummary>(`/api/posts/${postId}/like`, {
    method: "DELETE"
  });
}

export function favoritePost(postId: number): Promise<InteractionSummary> {
  return apiRequest<InteractionSummary>(`/api/posts/${postId}/favorite`, {
    method: "PUT"
  });
}

export function unfavoritePost(postId: number): Promise<InteractionSummary> {
  return apiRequest<InteractionSummary>(`/api/posts/${postId}/favorite`, {
    method: "DELETE"
  });
}

export function listMyFavoritePosts(page = 1, pageSize = 10): Promise<PagedPosts> {
  return apiRequest<PagedPosts>(`/api/me/favorites?page=${page}&pageSize=${pageSize}`, {
    method: "GET"
  });
}

export function listMyFavoritePostsWithQuery(
  page = 1,
  pageSize = 10,
  query = "",
  order: "asc" | "desc" = "desc"
): Promise<PagedPosts> {
  const params = new URLSearchParams();
  params.set("page", String(page));
  params.set("pageSize", String(pageSize));
  params.set("order", order);
  if (query.trim()) {
    params.set("q", query.trim());
  }
  return apiRequest<PagedPosts>(`/api/me/favorites?${params.toString()}`, {
    method: "GET"
  });
}

export function listComments(postId: number, page = 1, pageSize = 10): Promise<PagedComments> {
  return apiRequest<PagedComments>(`/api/posts/${postId}/comments?page=${page}&pageSize=${pageSize}`, {
    method: "GET",
    skipAuth: true
  });
}

export function createComment(postId: number, content: string): Promise<CommentItem> {
  return apiRequest<CommentItem>(`/api/posts/${postId}/comments`, {
    method: "POST",
    body: JSON.stringify({ content })
  });
}

export function deleteComment(commentId: number): Promise<{ deleted: boolean; id: number }> {
  return apiRequest<{ deleted: boolean; id: number }>(`/api/comments/${commentId}`, {
    method: "DELETE"
  });
}
