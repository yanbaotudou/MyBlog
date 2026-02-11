import { Navigate, Outlet } from "react-router-dom";
import { useAuthState } from "../store/authStore";

export function AdminRoute() {
  const auth = useAuthState();

  if (!auth.accessToken || !auth.user) {
    return <Navigate to="/login" replace />;
  }

  if (auth.user.role !== "admin") {
    return (
      <div className="empty-state">
        <h2>无权限</h2>
        <p>该页面仅管理员可访问。</p>
      </div>
    );
  }

  return <Outlet />;
}
