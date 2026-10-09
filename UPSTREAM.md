# 来源与组件

| 组件 | 来源 | 版本 | 许可证 |
| --- | --- | --- | --- |
| OV-Watch | [No-Chicken/OV-Watch](https://github.com/No-Chicken/OV-Watch) | `9b5806531214b50e4d039498f875b0f1b48808ed` | GPL-3.0 |
| LVGL | 上游工程内附源码 | 8.2 | MIT |
| SDL2 | [libsdl-org/SDL](https://github.com/libsdl-org/SDL/releases/tag/release-2.30.12) | 2.30.12 | zlib |

上游作者为 No-Chicken 及其贡献者。原始 STM32 固件、硬件设计和外壳模型保留在 `upstream/OV-Watch-main/`，没有修改原始测量算法。

## 扩展范围

- C11 健康模型：趋势缓冲、提醒状态机、运动目标和日历史。
- 数据持久化：小端编码、版本号、CRC32、语义校验和文件替换。
- LVGL 界面：健康与运动页面、深色主题、主页卡片、双列菜单和页面资源释放。
- Windows 构建：MSVC / CMake / Ninja，官方 SDL2 开发包及依赖校验。
- 验证工具：C 核心测试、异常文件、LVGL 控件事件和独立进程恢复。

桌面程序以 `lv_sim_vscode_win` 为基础，使用 `SDL_Delay` 和官方 SDL 头文件布局，补充初始化失败检查及显示缓冲导出功能。新增应用模块当前在 Windows 上验证，未完成 STM32 部署。

下载包 SHA-256 与固定提交记录在 `SOURCE_LOCK.json`。第三方文件中的许可证和版权声明应保留。
