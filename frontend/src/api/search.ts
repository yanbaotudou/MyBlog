import type { PagedPosts } from "../types/post";
import { apiRequest } from "./client";

export function searchPosts(q: string, page = 1, pageSize = 10): Promise<PagedPosts & { q: string }> {
  const query = encodeURIComponent(q);
  return apiRequest<PagedPosts & { q: string }>(
    `/api/search?q=${query}&page=${page}&pageSize=${pageSize}`,
    {
      method: "GET",
      skipAuth: true
    }
  );
}
