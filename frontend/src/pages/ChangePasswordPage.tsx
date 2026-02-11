import { FormEvent, useState } from "react";
import { useNavigate } from "react-router-dom";
import { changePassword } from "../api/auth";
import { authStore } from "../store/authStore";

export function ChangePasswordPage() {
  const navigate = useNavigate();
  const [currentPassword, setCurrentPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");

  const onSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError("");
    setSuccess("");

    if (newPassword.length < 8 || newPassword.length > 72) {
      setError("新密码长度需为 8-72 位");
      return;
    }

    if (newPassword !== confirmPassword) {
      setError("两次输入的新密码不一致");
      return;
    }

    if (currentPassword === newPassword) {
      setError("新密码不能与当前密码相同");
      return;
    }

    setLoading(true);
    try {
      const data = await changePassword({
        currentPassword,
        newPassword
      });
      authStore.setAuth(data.accessToken, data.user);
      setCurrentPassword("");
      setNewPassword("");
      setConfirmPassword("");
      setSuccess("密码修改成功，登录状态已更新");
      window.setTimeout(() => {
        navigate("/");
      }, 900);
    } catch (e) {
      setError(e instanceof Error ? e.message : "修改密码失败");
    } finally {
      setLoading(false);
    }
  };

  return (
    <main className="container auth-page">
      <section className="auth-card">
        <h1>修改密码</h1>
        <p>修改后将使旧 refresh token 失效，并签发新的登录凭证。</p>

        <form onSubmit={onSubmit}>
          <label htmlFor="current-password">当前密码</label>
          <input
            id="current-password"
            value={currentPassword}
            onChange={(e) => setCurrentPassword(e.target.value)}
            minLength={8}
            maxLength={72}
            type="password"
            required
          />

          <label htmlFor="new-password">新密码</label>
          <input
            id="new-password"
            value={newPassword}
            onChange={(e) => setNewPassword(e.target.value)}
            minLength={8}
            maxLength={72}
            type="password"
            required
          />

          <label htmlFor="confirm-password">确认新密码</label>
          <input
            id="confirm-password"
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
            minLength={8}
            maxLength={72}
            type="password"
            required
          />

          <button className="primary-btn" type="submit" disabled={loading}>
            {loading ? "提交中..." : "确认修改"}
          </button>
        </form>

        {error ? <p className="status-line error">{error}</p> : null}
        {success ? <p className="status-line">{success}</p> : null}
      </section>
    </main>
  );
}
