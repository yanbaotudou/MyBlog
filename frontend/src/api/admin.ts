import type { UserProfile } from "../types/user";
import { apiRequest } from "./client";

interface PagedUsers {
  items: UserProfile[];
  page: number;
  pageSize: number;
  total: number;
}

export function listUsers(page = 1, pageSize = 20): Promise<PagedUsers> {
  return apiRequest<PagedUsers>(`/api/admin/users?page=${page}&pageSize=${pageSize}`, {
    method: "GET"
  });
}

export function updateUserRole(userId: number, role: "user" | "admin"): Promise<UserProfile> {
  return apiRequest<UserProfile>(`/api/admin/users/${userId}/role`, {
    method: "PUT",
    body: JSON.stringify({ role })
  });
}

export function updateUserBan(userId: number, isBanned: boolean): Promise<UserProfile> {
  return apiRequest<UserProfile>(`/api/admin/users/${userId}/ban`, {
    method: "PUT",
    body: JSON.stringify({ isBanned })
  });
}
