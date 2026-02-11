import type { PagedPosts, Post } from "../types/post";
import { apiRequest } from "./client";

interface PostPayload {
  title: string;
  contentMarkdown: string;
}

export function listPosts(page = 1, pageSize = 10): Promise<PagedPosts> {
  return apiRequest<PagedPosts>(`/api/posts?page=${page}&pageSize=${pageSize}`, {
    method: "GET",
    skipAuth: true
  });
}

export function getPost(id: number): Promise<Post> {
  return apiRequest<Post>(`/api/posts/${id}`, {
    method: "GET",
    skipAuth: true
  });
}

export function createPost(payload: PostPayload): Promise<Post> {
  return apiRequest<Post>("/api/posts", {
    method: "POST",
    body: JSON.stringify(payload)
  });
}

export function updatePost(id: number, payload: PostPayload): Promise<Post> {
  return apiRequest<Post>(`/api/posts/${id}`, {
    method: "PUT",
    body: JSON.stringify(payload)
  });
}

export function deletePost(id: number): Promise<{ deleted: boolean; id: number }> {
  return apiRequest<{ deleted: boolean; id: number }>(`/api/posts/${id}`, {
    method: "DELETE"
  });
}
