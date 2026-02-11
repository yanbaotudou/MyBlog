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
  _retry?: boolean;
}

let refreshPromise: Promise<boolean> | null = null;

async function tryRefreshAccessToken(): Promise<boolean> {
  if (refreshPromise) {
    return refreshPromise;
  }

  refreshPromise = (async () => {
    try {
      const response = await fetch("/api/auth/refresh", {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        credentials: "include"
      });

      if (!response.ok) {
        return false;
      }

      const parsed = (await response.json()) as ApiSuccess<{
        accessToken: string;
        user: { id: number; username: string; role: "user" | "admin"; isBanned: boolean; createdAt: string };
      }>;

      if (!parsed?.data?.accessToken || !parsed?.data?.user) {
        return false;
      }

      authStore.setAuth(parsed.data.accessToken, parsed.data.user);
      return true;
    } catch {
      return false;
    } finally {
      refreshPromise = null;
    }
  })();

  return refreshPromise;
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

    const authError =
      response.status === 401 && (payload.code === "AUTH_REQUIRED" || payload.code === "AUTH_INVALID_TOKEN");

    if (!options.skipAuth && authError && !options._retry && path !== "/api/auth/refresh") {
      const refreshed = await tryRefreshAccessToken();
      if (refreshed) {
        return apiRequest<T>(path, {
          ...options,
          _retry: true
        });
      }
    }

    if (
      !options.skipAuth &&
      (authError || (response.status === 403 && payload.code === "USER_BANNED"))
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
