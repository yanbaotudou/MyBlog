import { useEffect, useState } from "react";
import { listUsers, updateUserBan, updateUserRole } from "../api/admin";
import { Pagination } from "../components/Pagination";
import type { UserProfile } from "../types/user";
import { formatAbsoluteDateTime } from "../utils/dateTime";

export function AdminUsersPage() {
  const [items, setItems] = useState<UserProfile[]>([]);
  const [page, setPage] = useState(1);
  const [total, setTotal] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const loadData = async (nextPage = page) => {
    setLoading(true);
    setError("");
    try {
      const data = await listUsers(nextPage, 20);
      setItems(data.items);
      setTotal(data.total);
    } catch (e) {
      setError(e instanceof Error ? e.message : "加载用户失败");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    void loadData(page);
  }, [page]);

  const onRoleChange = async (userId: number, role: "user" | "admin") => {
    try {
      await updateUserRole(userId, role);
      await loadData(page);
    } catch (e) {
      setError(e instanceof Error ? e.message : "更新角色失败");
    }
  };

  const onToggleBan = async (userId: number, current: boolean) => {
    try {
      await updateUserBan(userId, !current);
      await loadData(page);
    } catch (e) {
      setError(e instanceof Error ? e.message : "更新封禁状态失败");
    }
  };

  return (
    <main className="container admin-page">
      <h1>用户管理</h1>
      <p>管理员可以调整角色并封禁或解封用户。</p>

      {loading ? <p className="status-line">正在加载用户...</p> : null}
      {error ? <p className="status-line error">{error}</p> : null}

      <table className="admin-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>用户名</th>
            <th>创建时间</th>
            <th>角色</th>
            <th>状态</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          {items.map((user) => (
            <tr key={user.id}>
              <td>{user.id}</td>
              <td>{user.username}</td>
              <td>{formatAbsoluteDateTime(user.createdAt)}</td>
              <td>
                <select
                  value={user.role}
                  onChange={(e) => onRoleChange(user.id, e.target.value as "user" | "admin")}
                >
                  <option value="user">user</option>
                  <option value="admin">admin</option>
                </select>
              </td>
              <td>{user.isBanned ? "已封禁" : "正常"}</td>
              <td>
                <button className="secondary-btn" onClick={() => onToggleBan(user.id, user.isBanned)}>
                  {user.isBanned ? "解封" : "封禁"}
                </button>
              </td>
            </tr>
          ))}
        </tbody>
      </table>

      <Pagination page={page} pageSize={20} total={total} onPageChange={setPage} />
    </main>
  );
}
