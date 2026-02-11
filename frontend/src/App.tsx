import { useEffect, useState } from "react";
import { BrowserRouter, Route, Routes, useLocation } from "react-router-dom";
import { Toaster } from "sonner";
import { refresh } from "./api/auth";
import { AdminRoute } from "./components/AdminRoute";
import { Header } from "./components/Header";
import { ProtectedRoute } from "./components/ProtectedRoute";
import { AdminUsersPage } from "./pages/AdminUsersPage";
import { ChangePasswordPage } from "./pages/ChangePasswordPage";
import { CollectionDetailPage } from "./pages/CollectionDetailPage";
import { CollectionsPage } from "./pages/CollectionsPage";
import { EditorPage } from "./pages/EditorPage";
import { HomePage } from "./pages/HomePage";
import { LoginPage } from "./pages/LoginPage";
import { PostDetailPage } from "./pages/PostDetailPage";
import { ProfilePage } from "./pages/ProfilePage";
import { SearchPage } from "./pages/SearchPage";
import { authStore } from "./store/authStore";

function AppRoutes() {
  const location = useLocation();
  const hideHeader = location.pathname === "/login" || location.pathname.startsWith("/editor");

  return (
    <div className="min-h-screen bg-background">
      <Toaster position="top-center" richColors />
      {!hideHeader ? <Header /> : null}
      <main>
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/login" element={<LoginPage />} />
          <Route path="/posts/:id" element={<PostDetailPage />} />
          <Route path="/search" element={<SearchPage />} />
          <Route path="/collections/:id" element={<CollectionDetailPage />} />

          <Route element={<ProtectedRoute />}>
            <Route path="/editor/new" element={<EditorPage />} />
            <Route path="/editor/:id" element={<EditorPage />} />
            <Route path="/collections" element={<CollectionsPage />} />
            <Route path="/me" element={<ProfilePage />} />
            <Route path="/account/password" element={<ChangePasswordPage />} />
          </Route>

          <Route element={<AdminRoute />}>
            <Route path="/admin/users" element={<AdminUsersPage />} />
          </Route>
        </Routes>
      </main>
    </div>
  );
}

export default function App() {
  const [booting, setBooting] = useState(true);

  useEffect(() => {
    const run = async () => {
      const cached = authStore.getState();

      if (cached.accessToken && cached.user) {
        // Prefer cached login state for "remember me" UX, then silently refresh in background.
        setBooting(false);
        try {
          const data = await refresh();
          authStore.setAuth(data.accessToken, data.user);
        } catch {
          // Keep cached state; it'll be cleared only when real auth failures happen.
        }
        return;
      }

      try {
        const data = await refresh();
        authStore.setAuth(data.accessToken, data.user);
      } catch {
        authStore.clear();
      }
      setBooting(false);
    };

    void run();
  }, []);

  if (booting) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="animate-pulse text-muted-foreground">加载中...</div>
      </div>
    );
  }

  return (
    <BrowserRouter>
      <AppRoutes />
    </BrowserRouter>
  );
}
