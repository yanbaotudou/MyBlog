export interface ApiSuccess<T> {
  code: "OK";
  message: string;
  data: T;
  requestId: string;
}

export interface ApiErrorResponse {
  code: string;
  message: string;
  details?: Record<string, unknown>;
  requestId?: string;
}
