import type { Post } from "./post";

export interface Collection {
  id: number;
  name: string;
  description: string;
  ownerId: number;
  ownerUsername: string;
  createdAt: string;
  updatedAt: string;
  postCount: number;
}

export interface CollectionDetail {
  collection: Collection;
  posts: Post[];
  total: number;
}

export interface PostCollectionMembership {
  collectionId: number;
  collectionName: string;
  position: number;
}

export interface CollectionNavigation {
  collectionId: number;
  currentPosition: number;
  prev: Post | null;
  next: Post | null;
}

export interface PostCollectionsData {
  items: PostCollectionMembership[];
  navigation: CollectionNavigation | null;
}
