import { useSyncExternalStore } from "react";
import type { AuthState } from "../types/auth";
import type { UserProfile } from "../types/user";

const STORAGE_KEY = "study_blog_auth";

function loadInitialState(): AuthState {
  const raw = localStorage.getItem(STORAGE_KEY);
  if (!raw) {
    return { accessToken: null, user: null };
  }

  try {
    const parsed = JSON.parse(raw) as AuthState;
    if (!parsed.accessToken || !parsed.user) {
      return { accessToken: null, user: null };
    }
    return parsed;
  } catch {
    return { accessToken: null, user: null };
  }
}

class AuthStore {
  private state: AuthState = loadInitialState();
  private listeners = new Set<() => void>();

  private emit() {
    this.listeners.forEach((listener) => listener());
  }

  private persist() {
    if (!this.state.accessToken || !this.state.user) {
      localStorage.removeItem(STORAGE_KEY);
      return;
    }
    localStorage.setItem(STORAGE_KEY, JSON.stringify(this.state));
  }

  getState = (): AuthState => {
    return this.state;
  };

  getAccessToken = (): string | null => {
    return this.state.accessToken;
  };

  setAuth = (accessToken: string, user: UserProfile) => {
    this.state = { accessToken, user };
    this.persist();
    this.emit();
  };

  clear = () => {
    this.state = { accessToken: null, user: null };
    this.persist();
    this.emit();
  };

  subscribe = (listener: () => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };
}

export const authStore = new AuthStore();

export function useAuthState(): AuthState {
  return useSyncExternalStore(authStore.subscribe, authStore.getState, authStore.getState);
}
