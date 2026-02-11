import { apiRequest } from "./client";
import type { CommentItem, InteractionSummary, PagedComments } from "../types/interaction";

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
