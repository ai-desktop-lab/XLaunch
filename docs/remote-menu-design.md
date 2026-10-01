# XLaunch 简约菜单交付规范

## 目标与视觉

保留现有琥珀面板、系统 APP logo、分类侧栏与全屏应用网格。简约模式收紧为 520 × 560 逻辑像素（受工作区尺寸限制），左边对齐屏幕、底边对齐 Dock 预留区域。遵循 QScreen availableGeometry，屏幕/占位尺寸变动后重新定位。无窗口装饰，不占任务栏。

菜单间距 18 px，内容区间距 12 px；标题 22 px，行文字 14 px，应用行 42 px，分类行 34 px，搜索框 40 px。琥珀 token：surface `#513b27`、base `#3f2f21`、border `#756047`、hover `#604b35`、selection `#796044`、focus `#dfbf8d`。跟随系统主题时使用对应 Qt palette 角色。

## 交互

- 顶部直接搜索应用，分类切换更新右侧应用列表。
- 点击应用或 Enter 启动；启动异步执行，避免 UI 等待 gio。重复启动请求在当前请求完成前不接受。
- 右键应用“在 Dock 中驻留”，与同一显示会话的 XDock 通信。
- 单击“全部应用”或 F11 切换到覆盖整个屏幕的全屏网格；Menu、Esc 返回简约模式，Esc 再次关闭。
- 搜索结果为空显示明确空状态；启动/驻留失败在底部显示原因。
- 文本溢出省略，悬停显示完整应用名。方向键、Enter、Tab、Ctrl+F 提供键盘操作。
- 每个显示会话单实例。`xlaunch --toggle` 切换菜单，`--hidden` 后台常驻，`--fullscreen` 首次启动进入网格。
- 点击其他应用时简约菜单隐藏；自己的右键弹出菜单不关闭主菜单。

## 有限资源策略

菜单 ListView 虚拟化并复用行。全屏模式隐藏时不创建页面 delegate；菜单隐藏到全屏时不创建菜单应用行。全屏仅加载视口附近页面，使用静态背景图片，不使用实时模糊；翻页定位不播放过渡。应用图标按需加载并缓存，空图标使用系统 fallback。远程会话建议 `QT_QUICK_BACKEND=software`，由会话配置控制。

窗口枚举、驻留状态和排序由 XDock 负责；XLaunch 只发送有效桌面文件，不维护另一份驻留列表。当前 IPC 配对要求两端同时更新到会话隔离版本。

## KDE X11 全屏切换修正（2026-10-01）

使用普通无边框窗口类型，允许 KWin 接受真正的 FULLSCREEN 状态。菜单尺寸使用 availableGeometry（避开 Dock），全屏使用 screen geometry（覆盖整个屏幕）。Qt 切换窗口状态后，通过 EWMH 请求保留 SKIP_TASKBAR / SKIP_PAGER。

Home-Ubuntu 的 DISPLAY :10.0 实际单击“全部应用”：1352×848 +0+0，存在 _NET_WM_STATE_FULLSCREEN。Esc 返回 520×560 +0+210，底边与 Dock 顶边 770 对齐。实际屏幕抓图确认是全屏琥珀应用网格，未停留在菜单窗口大小。已安装到 ~/.local/bin/xlaunch，远程会话日志无 QML 错误。
