import { Navigate, Outlet, useLocation } from "react-router-dom";
import { useAuthState } from "../store/authStore";

export function ProtectedRoute() {
  const auth = useAuthState();
  const location = useLocation();

  if (!auth.accessToken || !auth.user) {
    return <Navigate to="/login" replace state={{ from: location }} />;
  }

  return <Outlet />;
}
