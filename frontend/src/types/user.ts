export type Role = "user" | "admin";

export interface UserProfile {
  id: number;
  username: string;
  role: Role;
  isBanned: boolean;
  createdAt: string;
}
