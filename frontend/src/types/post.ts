export interface Post {
  id: number;
  title: string;
  contentMarkdown: string;
  authorId: number;
  authorUsername: string;
  createdAt: string;
  updatedAt: string;
  isDeleted: boolean;
  collectionPosition?: number;
}

export interface PagedPosts {
  items: Post[];
  page: number;
  pageSize: number;
  total: number;
}
