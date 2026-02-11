import { useEffect, useState } from "react";
import { BrowserRouter, Route, Routes } from "react-router-dom";
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
import { SearchPage } from "./pages/SearchPage";
import { authStore } from "./store/authStore";

export default function App() {
  const [booting, setBooting] = useState(true);

  useEffect(() => {
    const run = async () => {
      try {
        const data = await refresh();
        authStore.setAuth(data.accessToken, data.user);
      } catch {
        authStore.clear();
      } finally {
        setBooting(false);
      }
    };

    void run();
  }, []);

  if (booting) {
    return <div className="boot-screen">加载中...</div>;
  }

  return (
    <BrowserRouter>
      <div className="app-shell">
        <Header />
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
            <Route path="/account/password" element={<ChangePasswordPage />} />
          </Route>

          <Route element={<AdminRoute />}>
            <Route path="/admin/users" element={<AdminUsersPage />} />
          </Route>
        </Routes>
      </div>
    </BrowserRouter>
  );
}
