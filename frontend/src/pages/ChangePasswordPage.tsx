import { useState } from "react";
import { ArrowLeft, CheckCircle, Eye, EyeOff, Lock, Shield } from "lucide-react";
import { useNavigate } from "react-router-dom";
import { changePassword } from "../api/auth";
import { authStore } from "../store/authStore";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Progress } from "@/components/ui/progress";

function calculatePasswordStrength(password: string): number {
  let strength = 0;
  if (password.length >= 8) strength += 25;
  if (password.match(/[a-z]/)) strength += 25;
  if (password.match(/[A-Z]/)) strength += 25;
  if (password.match(/[0-9!@#$%^&*]/)) strength += 25;
  return strength;
}

function getStrengthLabel(strength: number): string {
  if (strength <= 25) return "弱";
  if (strength <= 50) return "中";
  if (strength <= 75) return "强";
  return "非常强";
}

export function ChangePasswordPage() {
  const navigate = useNavigate();
  const [currentPassword, setCurrentPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [showCurrentPassword, setShowCurrentPassword] = useState(false);
  const [showNewPassword, setShowNewPassword] = useState(false);
  const [showConfirmPassword, setShowConfirmPassword] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [isSuccess, setIsSuccess] = useState(false);

  const strength = calculatePasswordStrength(newPassword);

  const onSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError("");

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
      const data = await changePassword({ currentPassword, newPassword });
      authStore.setAuth(data.accessToken, data.user);
      setIsSuccess(true);
    } catch (e) {
      setError(e instanceof Error ? e.message : "修改密码失败");
    } finally {
      setLoading(false);
    }
  };

  if (isSuccess) {
    return (
      <div className="min-h-screen bg-gradient-to-b from-primary/5 to-background flex items-center justify-center py-12 px-4 sm:px-6 lg:px-8">
        <Card className="w-full max-w-md shadow-xl">
          <CardContent className="pt-8 pb-8 text-center">
            <div className="w-16 h-16 bg-green-100 rounded-full flex items-center justify-center mx-auto mb-4">
              <CheckCircle className="w-8 h-8 text-green-600" />
            </div>
            <h2 className="text-2xl font-bold text-foreground mb-2">密码修改成功</h2>
            <p className="text-muted-foreground mb-6">你的密码已成功更新，登录状态已刷新</p>
            <Button className="w-full" onClick={() => navigate("/")}>
              返回首页
            </Button>
          </CardContent>
        </Card>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-b from-primary/5 to-background py-12 px-4 sm:px-6 lg:px-8">
      <div className="max-w-md mx-auto">
        <Button variant="ghost" size="sm" className="gap-2 mb-6" onClick={() => navigate("/")}>
          <ArrowLeft className="w-4 h-4" />
          返回
        </Button>

        <Card className="shadow-xl">
          <CardHeader className="space-y-1">
            <div className="w-12 h-12 bg-primary/10 rounded-xl flex items-center justify-center mb-4">
              <Shield className="w-6 h-6 text-primary" />
            </div>
            <CardTitle className="text-2xl">修改密码</CardTitle>
            <CardDescription>为了账户安全，请定期更换密码</CardDescription>
          </CardHeader>
          <CardContent>
            <form onSubmit={onSubmit} className="space-y-6">
              <div className="space-y-2">
                <Label htmlFor="current-password">当前密码</Label>
                <div className="relative">
                  <Lock className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
                  <Input
                    id="current-password"
                    type={showCurrentPassword ? "text" : "password"}
                    value={currentPassword}
                    onChange={(e) => setCurrentPassword(e.target.value)}
                    className="pl-10 pr-10"
                    required
                  />
                  <Button
                    type="button"
                    variant="ghost"
                    size="icon"
                    className="absolute right-0 top-0 h-full px-3"
                    onClick={() => setShowCurrentPassword(!showCurrentPassword)}
                  >
                    {showCurrentPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
                  </Button>
                </div>
              </div>

              <div className="space-y-2">
                <Label htmlFor="new-password">新密码</Label>
                <div className="relative">
                  <Lock className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
                  <Input
                    id="new-password"
                    type={showNewPassword ? "text" : "password"}
                    value={newPassword}
                    onChange={(e) => setNewPassword(e.target.value)}
                    className="pl-10 pr-10"
                    minLength={8}
                    maxLength={72}
                    required
                  />
                  <Button
                    type="button"
                    variant="ghost"
                    size="icon"
                    className="absolute right-0 top-0 h-full px-3"
                    onClick={() => setShowNewPassword(!showNewPassword)}
                  >
                    {showNewPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
                  </Button>
                </div>
                {newPassword ? (
                  <div className="space-y-2">
                    <div className="flex items-center justify-between text-sm">
                      <span className="text-muted-foreground">密码强度</span>
                      <span className="font-medium">{getStrengthLabel(strength)}</span>
                    </div>
                    <Progress value={strength} className="h-2" />
                  </div>
                ) : null}
              </div>

              <div className="space-y-2">
                <Label htmlFor="confirm-password">确认新密码</Label>
                <div className="relative">
                  <Lock className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
                  <Input
                    id="confirm-password"
                    type={showConfirmPassword ? "text" : "password"}
                    value={confirmPassword}
                    onChange={(e) => setConfirmPassword(e.target.value)}
                    className="pl-10 pr-10"
                    required
                  />
                  <Button
                    type="button"
                    variant="ghost"
                    size="icon"
                    className="absolute right-0 top-0 h-full px-3"
                    onClick={() => setShowConfirmPassword(!showConfirmPassword)}
                  >
                    {showConfirmPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
                  </Button>
                </div>
              </div>

              {error ? <p className="text-sm text-destructive">{error}</p> : null}

              <Button type="submit" className="w-full btn-hover-lift" disabled={loading}>
                {loading ? <span className="animate-pulse">修改中...</span> : "确认修改"}
              </Button>
            </form>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
