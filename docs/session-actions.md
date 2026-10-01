# 用户与会话菜单

## 默认界面

XLaunch 简约菜单右下角仅保留两个 32px 图标：收起箭头和用户头像/首字母。悬停显示名称和操作说明，登录、注销、关机等只在 240px 下拉中出现一次；下拉向上展开并右对齐，应用数量保留在左侧。菜单项明确设置浅色文本、禁用文本和琥珀选中背景，避免 Qt 主题继承造成黑字。XDock 右侧提供用户头像（缺失时显示首字母）。两处进入相同的用户信息、锁屏、登录/切换用户、注销、重启主机、关闭主机菜单。保留琥珀色层次和现有应用布局；关闭 XLaunch 菜单与关闭主机使用不同标签。

注销、远程重新登录、重启、关机先出现确认窗口，默认键盘焦点为“取消”。确认内容明确当前桌面或整台主机的影响。Esc 关闭确认，不执行操作。不支持的动作禁用并提供原因；不会自动调用 sudo 或强制关机。

## Linux 路由

- 用户名称由当前 UID 的 passwd 信息取得，头像读取 .face / AccountsService 的用户头像。
- KDE 锁屏使用当前会话 D-Bus 的 org.freedesktop.ScreenSaver.Lock；独立 X11 可选 xsecurelock。仅发 logind Lock 信号不等同于实际锁屏，因此不作为锁屏成功的替代实现。
- KDE 注销使用当前会话 org.kde.Shutdown.logout；重启/关机使用 logoutAndReboot / logoutAndShutdown，交由桌面保存应用和系统权限处理。
- IceWM 注销使用 icesh logout。其他环境仅在 logind 会话的 UID、图形类型、DISPLAY 匹配后，允许 TerminateSession；绝不 TerminateUser。
- 本地登录/切换用户：已验证本地 seat，通过 DisplayManager.CanSwitch 查询能力；先锁定当前会话，再 SwitchToGreeter。多 seat 无法确定目标时禁用。
- RDP 无物理 seat：登录/切换用户提示注销当前远程桌面，再由客户端选择账号重新连接，不切换主机物理登录界面。
- 无 KDE 的重启/关机调用 logind Reboot / PowerOff，interactive=true；CanReboot / CanPowerOff 为 yes 或 challenge 才可用。无 logind 的容器禁用主机电源动作。
- 非 Linux 平台暂不提供这些系统动作；用户信息可查看。

能力在启动和打开菜单时异步刷新，没有空闲轮询。两仓库的 SessionActions.h/.cpp 为相同实现，修改时同步更新；Qt DBus 只在 Linux 构建链接。

## 交付与验证范围

远端 Qt 构建、git diff --check 和菜单渲染检查；检查确认窗口时只取消操作。本次不实际注销、锁屏、重启或关机作为验收。真实会话断开与系统电源变化仍需用户在合适时间主动验收。当前部署使用 ~/.local/bin，程序替换前保留原始备份，ISO / Debian 包尚未重新发布。

## 接口依据

- [systemd logind D-Bus API](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml)
- [KDE SessionManagement 接口](https://github.com/KDE/plasma-workspace/blob/master/libkworkspace/sessionmanagement.h)
- IceWM icesh 本机手册的 logout 动作；DisplayManager seat 接口和 KDE Shutdown 方法已通过目标主机只读 introspection 确认。
