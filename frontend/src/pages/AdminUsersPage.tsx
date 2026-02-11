import { useEffect, useMemo, useState } from "react";
import { Ban, Shield, ShieldCheck, User, Users } from "lucide-react";
import { listUsers, updateUserBan, updateUserRole } from "../api/admin";
import type { UserProfile } from "../types/user";
import { formatAbsoluteDateTime } from "../utils/dateTime";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";

const PAGE_SIZE = 20;

export function AdminUsersPage() {
  const [items, setItems] = useState<UserProfile[]>([]);
  const [page, setPage] = useState(1);
  const [total, setTotal] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [pendingUserId, setPendingUserId] = useState<number | null>(null);

  const pageCount = useMemo(() => Math.max(1, Math.ceil(total / PAGE_SIZE)), [total]);

  const loadData = async (nextPage = page) => {
    setLoading(true);
    setError("");
    try {
      const data = await listUsers(nextPage, PAGE_SIZE);
      setItems(data.items);
      setTotal(data.total);
      setPage(data.page);
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
    setPendingUserId(userId);
    setError("");
    try {
      await updateUserRole(userId, role);
      await loadData(page);
    } catch (e) {
      setError(e instanceof Error ? e.message : "更新角色失败");
    } finally {
      setPendingUserId(null);
    }
  };

  const onToggleBan = async (userId: number, current: boolean) => {
    setPendingUserId(userId);
    setError("");
    try {
      await updateUserBan(userId, !current);
      await loadData(page);
    } catch (e) {
      setError(e instanceof Error ? e.message : "更新封禁状态失败");
    } finally {
      setPendingUserId(null);
    }
  };

  const adminCount = items.filter((item) => item.role === "admin").length;
  const bannedCount = items.filter((item) => item.isBanned).length;

  return (
    <div className="min-h-screen bg-background">
      <div className="border-b border-border">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
          <h1 className="text-2xl font-bold text-foreground">用户管理</h1>
          <p className="text-muted-foreground mt-1">管理员可以调整角色并封禁或解封用户。</p>
          {error ? <p className="text-sm text-destructive mt-3">{error}</p> : null}
        </div>
      </div>

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6 space-y-6">
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <Card>
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-muted-foreground">当前页用户</CardTitle>
            </CardHeader>
            <CardContent className="flex items-end justify-between">
              <div className="text-2xl font-bold">{items.length}</div>
              <Users className="w-5 h-5 text-muted-foreground" />
            </CardContent>
          </Card>
          <Card>
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-muted-foreground">管理员数量</CardTitle>
            </CardHeader>
            <CardContent className="flex items-end justify-between">
              <div className="text-2xl font-bold">{adminCount}</div>
              <ShieldCheck className="w-5 h-5 text-muted-foreground" />
            </CardContent>
          </Card>
          <Card>
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-muted-foreground">封禁用户</CardTitle>
            </CardHeader>
            <CardContent className="flex items-end justify-between">
              <div className="text-2xl font-bold">{bannedCount}</div>
              <Ban className="w-5 h-5 text-muted-foreground" />
            </CardContent>
          </Card>
        </div>

        <Card>
          <CardContent className="pt-6">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>ID</TableHead>
                  <TableHead>用户</TableHead>
                  <TableHead>创建时间</TableHead>
                  <TableHead>角色</TableHead>
                  <TableHead>状态</TableHead>
                  <TableHead className="text-right">操作</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {loading ? (
                  <TableRow>
                    <TableCell colSpan={6} className="text-center text-muted-foreground py-8">
                      正在加载用户...
                    </TableCell>
                  </TableRow>
                ) : items.length === 0 ? (
                  <TableRow>
                    <TableCell colSpan={6} className="text-center text-muted-foreground py-8">
                      暂无数据
                    </TableCell>
                  </TableRow>
                ) : (
                  items.map((user) => {
                    const pending = pendingUserId === user.id;
                    return (
                      <TableRow key={user.id}>
                        <TableCell>{user.id}</TableCell>
                        <TableCell>
                          <div className="flex items-center gap-2">
                            <User className="w-4 h-4 text-muted-foreground" />
                            <span className="font-medium">{user.username}</span>
                          </div>
                        </TableCell>
                        <TableCell>{formatAbsoluteDateTime(user.createdAt)}</TableCell>
                        <TableCell>
                          <Select
                            value={user.role}
                            disabled={pending}
                            onValueChange={(value) => onRoleChange(user.id, value as "user" | "admin")}
                          >
                            <SelectTrigger className="w-[120px]">
                              <SelectValue />
                            </SelectTrigger>
                            <SelectContent>
                              <SelectItem value="user">user</SelectItem>
                              <SelectItem value="admin">admin</SelectItem>
                            </SelectContent>
                          </Select>
                        </TableCell>
                        <TableCell>
                          {user.isBanned ? (
                            <Badge variant="destructive">已封禁</Badge>
                          ) : (
                            <Badge variant="secondary">正常</Badge>
                          )}
                        </TableCell>
                        <TableCell className="text-right">
                          <Button
                            size="sm"
                            variant={user.isBanned ? "secondary" : "destructive"}
                            disabled={pending}
                            onClick={() => onToggleBan(user.id, user.isBanned)}
                          >
                            {pending ? "处理中..." : user.isBanned ? "解封" : "封禁"}
                          </Button>
                        </TableCell>
                      </TableRow>
                    );
                  })
                )}
              </TableBody>
            </Table>
          </CardContent>
        </Card>

        <div className="flex items-center justify-between">
          <p className="text-sm text-muted-foreground">
            第 {page} / {pageCount} 页，共 {total} 个用户
          </p>
          <div className="flex items-center gap-2">
            <Button variant="outline" size="sm" onClick={() => setPage((p) => Math.max(1, p - 1))} disabled={page <= 1}>
              上一页
            </Button>
            <Button
              variant="outline"
              size="sm"
              onClick={() => setPage((p) => Math.min(pageCount, p + 1))}
              disabled={page >= pageCount}
            >
              下一页
            </Button>
          </div>
        </div>
      </div>
    </div>
  );
}
