import {
  BookOpen,
  FileText,
  LogOut,
  Menu,
  PenLine,
  Search,
  Settings,
  Shield
} from "lucide-react";
import type { ComponentType } from "react";
import { Link, NavLink, useLocation, useNavigate } from "react-router-dom";
import { logout } from "../api/auth";
import { authStore, useAuthState } from "../store/authStore";
import { Avatar, AvatarFallback } from "./ui/avatar";
import { Button } from "./ui/button";
import { Sheet, SheetContent, SheetTrigger } from "./ui/sheet";

interface NavEntry {
  to: string;
  label: string;
  icon: ComponentType<{ className?: string }>;
  requireAuth?: boolean;
  requireAdmin?: boolean;
}

const NAV_ITEMS: NavEntry[] = [
  { to: "/", label: "首页", icon: FileText },
  { to: "/me", label: "个人中心", icon: Settings, requireAuth: true },
  { to: "/editor/new", label: "写文章", icon: PenLine, requireAuth: true },
  { to: "/collections", label: "合集", icon: BookOpen, requireAuth: true },
  { to: "/admin/users", label: "管理", icon: Shield, requireAdmin: true }
];

export function Header() {
  const auth = useAuthState();
  const navigate = useNavigate();
  const location = useLocation();

  const visibleItems = NAV_ITEMS.filter((item) => {
    if (item.requireAdmin) {
      return auth.user?.role === "admin";
    }
    if (item.requireAuth) {
      return !!auth.user;
    }
    return true;
  });

  const onLogout = async () => {
    try {
      await logout();
    } finally {
      authStore.clear();
      navigate("/");
    }
  };

  return (
    <header className="sticky top-0 z-50 w-full bg-background/85 backdrop-blur-md border-b border-border">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          <Link to="/" className="flex items-center gap-2">
            <div className="w-8 h-8 bg-primary rounded-lg flex items-center justify-center">
              <PenLine className="w-5 h-5 text-white" />
            </div>
            <span className="text-xl font-bold text-foreground">WriteSpace</span>
          </Link>

          <nav className="hidden md:flex items-center gap-1">
            {visibleItems.map((item) => {
              const Icon = item.icon;
              return (
                <NavLink key={item.to} to={item.to}>
                  {({ isActive }) => (
                    <Button
                      variant="ghost"
                      size="sm"
                      className={`gap-2 ${
                        isActive ? "text-primary bg-primary/10" : "text-muted-foreground hover:text-foreground"
                      }`}
                    >
                      <Icon className="w-4 h-4" />
                      {item.label}
                    </Button>
                  )}
                </NavLink>
              );
            })}
          </nav>

          <div className="flex items-center gap-2">
            <Button variant="ghost" size="icon" onClick={() => navigate("/search")} aria-label="搜索">
              <Search className="w-5 h-5" />
            </Button>

            {auth.user ? (
              <div className="flex items-center gap-1">
                <Link to="/me" className="rounded-full">
                  <Avatar className="h-9 w-9 ring-2 ring-transparent hover:ring-primary/30 transition-all">
                    <AvatarFallback className="bg-primary/10 text-primary">
                      {auth.user.username.charAt(0).toUpperCase()}
                    </AvatarFallback>
                  </Avatar>
                </Link>
              </div>
            ) : (
              <Button size="sm" className="btn-hover-lift" onClick={() => navigate("/login")}>
                登录 / 注册
              </Button>
            )}

            <Sheet>
              <SheetTrigger asChild>
                <Button variant="ghost" size="icon" className="md:hidden">
                  <Menu className="w-5 h-5" />
                </Button>
              </SheetTrigger>
              <SheetContent side="right" className="w-72">
                <div className="flex flex-col gap-4 mt-8">
                  {visibleItems.map((item) => {
                    const Icon = item.icon;
                    const active = location.pathname === item.to;
                    return (
                      <Button
                        key={item.to}
                        variant={active ? "default" : "ghost"}
                        className="justify-start gap-3"
                        onClick={() => navigate(item.to)}
                      >
                        <Icon className="w-5 h-5" />
                        {item.label}
                      </Button>
                    );
                  })}

                  {!auth.user ? (
                    <Button className="mt-4" onClick={() => navigate("/login")}>
                      登录 / 注册
                    </Button>
                  ) : (
                    <>
                      <Button variant="outline" className="justify-start gap-3" onClick={() => navigate("/account/password")}>
                        <Settings className="w-5 h-5" />
                        修改密码
                      </Button>
                      <Button variant="destructive" className="justify-start gap-3" onClick={onLogout}>
                        <LogOut className="w-5 h-5" />
                        退出登录
                      </Button>
                    </>
                  )}
                </div>
              </SheetContent>
            </Sheet>
          </div>
        </div>
      </div>
    </header>
  );
}
