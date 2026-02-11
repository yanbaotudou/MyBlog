import { FormEvent, useMemo, useState } from "react";
import { Link, NavLink, useLocation, useNavigate } from "react-router-dom";
import { logout } from "../api/auth";
import { authStore, useAuthState } from "../store/authStore";

export function Header() {
  const auth = useAuthState();
  const navigate = useNavigate();
  const location = useLocation();
  const [q, setQ] = useState("");
  const [busy, setBusy] = useState(false);

  const currentSearch = useMemo(() => {
    const params = new URLSearchParams(location.search);
    return params.get("q") || "";
  }, [location.search]);

  const handleSearch = (e: FormEvent) => {
    e.preventDefault();
    const query = q.trim() || currentSearch;
    if (!query) {
      return;
    }
    navigate(`/search?q=${encodeURIComponent(query)}`);
  };

  const handleLogout = async () => {
    setBusy(true);
    try {
      await logout();
    } finally {
      authStore.clear();
      setBusy(false);
      navigate("/");
    }
  };

  return (
    <header className="site-header">
      <div className="brand-block">
        <Link to="/" className="brand-title">
          学习日志
        </Link>
        <span className="brand-tag">Daily Build, Daily Note</span>
      </div>

      <form className="search-form" onSubmit={handleSearch}>
        <input
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="搜索标题或正文..."
          aria-label="搜索"
        />
        <button type="submit">搜索</button>
      </form>

      <nav className="nav-links">
        <NavLink to="/" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
          首页
        </NavLink>
        {auth.user ? (
          <NavLink to="/editor/new" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            写文章
          </NavLink>
        ) : (
          <NavLink to="/login" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            登录后写文章
          </NavLink>
        )}
        {auth.user?.role === "admin" ? (
          <NavLink to="/admin/users" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            管理后台
          </NavLink>
        ) : null}
        {auth.user ? (
          <NavLink to="/collections" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            合集
          </NavLink>
        ) : null}
        {auth.user ? (
          <NavLink to="/account/password" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            修改密码
          </NavLink>
        ) : null}
        {auth.user ? (
          <button onClick={handleLogout} disabled={busy} className="link-button" type="button">
            退出 · {auth.user.username}
          </button>
        ) : (
          <NavLink to="/login" className={({ isActive }) => (isActive ? "nav-item active" : "nav-item")}>
            登录 / 注册
          </NavLink>
        )}
      </nav>
    </header>
  );
}
