export interface InteractionSummary {
  likeCount: number;
  favoriteCount: number;
  commentCount: number;
  likedByMe: boolean;
  favoritedByMe: boolean;
}

export interface CommentItem {
  id: number;
  postId: number;
  userId: number;
  username: string;
  content: string;
  createdAt: string;
  updatedAt: string;
  isDeleted: boolean;
}

export interface PagedComments {
  items: CommentItem[];
  page: number;
  pageSize: number;
  total: number;
}
