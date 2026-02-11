import { FormEvent, useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import { login, register } from "../api/auth";
import { authStore } from "../store/authStore";

export function LoginPage() {
  const navigate = useNavigate();
  const location = useLocation();

  const [mode, setMode] = useState<"login" | "register">("login");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const redirectTo = (location.state as { from?: { pathname: string } } | null)?.from?.pathname || "/";

  const onSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setError("");

    try {
      const payload = { username: username.trim(), password };
      const data = mode === "login" ? await login(payload) : await register(payload);
      authStore.setAuth(data.accessToken, data.user);
      navigate(redirectTo, { replace: true });
    } catch (err) {
      setError(err instanceof Error ? err.message : "登录失败");
    } finally {
      setLoading(false);
    }
  };

  return (
    <main className="container auth-page">
      <section className="auth-card">
        <h1>{mode === "login" ? "登录" : "注册"}</h1>
        <p>未登录仅可浏览。登录后可发帖、编辑自己的文章。</p>

        <form onSubmit={onSubmit}>
          <label htmlFor="username">用户名</label>
          <input
            id="username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            minLength={3}
            maxLength={32}
            required
          />

          <label htmlFor="password">密码</label>
          <input
            id="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            minLength={8}
            maxLength={72}
            type="password"
            required
          />

          <button className="primary-btn" type="submit" disabled={loading}>
            {loading ? "提交中..." : mode === "login" ? "登录" : "注册"}
          </button>
        </form>

        {error ? <p className="status-line error">{error}</p> : null}

        <div className="toggle-row">
          {mode === "login" ? (
            <button type="button" className="link-button" onClick={() => setMode("register")}>
              没有账号？去注册
            </button>
          ) : (
            <button type="button" className="link-button" onClick={() => setMode("login")}>
              已有账号？去登录
            </button>
          )}
        </div>

        <div className="auth-tip">
          <strong>默认管理员：</strong>
          <span>admin / ChangeMe123!（首次启动后请立即修改）</span>
        </div>
      </section>
    </main>
  );
}
