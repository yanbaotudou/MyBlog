import { apiRequest } from "./client";
import type { Collection, CollectionDetail, PostCollectionsData } from "../types/collection";

export function createCollection(payload: { name: string; description: string }): Promise<Collection> {
  return apiRequest<Collection>("/api/collections", {
    method: "POST",
    body: JSON.stringify(payload)
  });
}

export function listMyCollections(): Promise<{ items: Collection[] }> {
  return apiRequest<{ items: Collection[] }>("/api/collections/mine", {
    method: "GET"
  });
}

export function getCollection(collectionId: number): Promise<CollectionDetail> {
  return apiRequest<CollectionDetail>(`/api/collections/${collectionId}`, {
    method: "GET",
    skipAuth: true
  });
}

export function addPostToCollection(collectionId: number, postId: number): Promise<{ added: boolean }> {
  return apiRequest<{ added: boolean }>(`/api/collections/${collectionId}/posts`, {
    method: "POST",
    body: JSON.stringify({ postId })
  });
}

export function removePostFromCollection(
  collectionId: number,
  postId: number
): Promise<{ removed: boolean; collectionId: number; postId: number }> {
  return apiRequest<{ removed: boolean; collectionId: number; postId: number }>(
    `/api/collections/${collectionId}/posts/${postId}`,
    {
      method: "DELETE"
    }
  );
}

export function getPostCollections(postId: number, collectionId?: number): Promise<PostCollectionsData> {
  const suffix = collectionId ? `?collectionId=${collectionId}` : "";
  return apiRequest<PostCollectionsData>(`/api/posts/${postId}/collections${suffix}`, {
    method: "GET",
    skipAuth: true
  });
}
