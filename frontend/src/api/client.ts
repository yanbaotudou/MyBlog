import { authStore } from "../store/authStore";
import type { ApiErrorResponse, ApiSuccess } from "../types/api";

export class ApiError extends Error {
  status: number;
  code: string;
  details?: Record<string, unknown>;
  requestId?: string;

  constructor(status: number, payload: ApiErrorResponse) {
    super(payload.message || "Request failed");
    this.status = status;
    this.code = payload.code || "UNKNOWN_ERROR";
    this.details = payload.details;
    this.requestId = payload.requestId;
  }
}

interface RequestOptions extends RequestInit {
  skipAuth?: boolean;
}

export async function apiRequest<T>(path: string, options: RequestOptions = {}): Promise<T> {
  const headers = new Headers(options.headers || {});
  headers.set("Content-Type", "application/json");

  if (!options.skipAuth) {
    const token = authStore.getAccessToken();
    if (token) {
      headers.set("Authorization", `Bearer ${token}`);
    }
  }

  const response = await fetch(path, {
    ...options,
    headers,
    credentials: "include"
  });

  if (response.status === 204) {
    return {} as T;
  }

  let parsed: ApiSuccess<T> | ApiErrorResponse | null = null;
  try {
    parsed = (await response.json()) as ApiSuccess<T> | ApiErrorResponse;
  } catch {
    parsed = null;
  }

  if (!response.ok) {
    const payload: ApiErrorResponse = parsed
      ? (parsed as ApiErrorResponse)
      : { code: "UNKNOWN_ERROR", message: "Request failed" };

    if (
      !options.skipAuth &&
      ((response.status === 401 && (payload.code === "AUTH_REQUIRED" || payload.code === "AUTH_INVALID_TOKEN")) ||
        (response.status === 403 && payload.code === "USER_BANNED"))
    ) {
      authStore.clear();
    }

    throw new ApiError(response.status, payload);
  }

  if (!parsed || !(parsed as ApiSuccess<T>).data) {
    throw new ApiError(response.status, {
      code: "INVALID_RESPONSE",
      message: "Server response format is invalid"
    });
  }

  return (parsed as ApiSuccess<T>).data;
}
