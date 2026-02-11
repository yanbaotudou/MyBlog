import type { UserProfile } from "./user";

export interface AuthPayload {
  accessToken: string;
  user: UserProfile;
}

export interface AuthState {
  accessToken: string | null;
  user: UserProfile | null;
}
