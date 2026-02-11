import type { AuthPayload } from "../types/auth";
import { apiRequest } from "./client";

interface Credentials {
  username: string;
  password: string;
}

export function register(payload: Credentials): Promise<AuthPayload> {
  return apiRequest<AuthPayload>("/api/auth/register", {
    method: "POST",
    body: JSON.stringify(payload),
    skipAuth: true
  });
}

export function login(payload: Credentials): Promise<AuthPayload> {
  return apiRequest<AuthPayload>("/api/auth/login", {
    method: "POST",
    body: JSON.stringify(payload),
    skipAuth: true
  });
}

export function refresh(): Promise<AuthPayload> {
  return apiRequest<AuthPayload>("/api/auth/refresh", {
    method: "POST",
    skipAuth: true
  });
}

export function logout(): Promise<{ ok: boolean }> {
  return apiRequest<{ ok: boolean }>("/api/auth/logout", {
    method: "POST",
    skipAuth: true
  });
}

export function changePassword(payload: { currentPassword: string; newPassword: string }): Promise<AuthPayload> {
  return apiRequest<AuthPayload>("/api/auth/change-password", {
    method: "POST",
    body: JSON.stringify(payload)
  });
}
