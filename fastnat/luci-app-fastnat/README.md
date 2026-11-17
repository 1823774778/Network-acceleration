# luci-app-fastnat

把 FanchmWrt「网络设置 → 网络加速」整条链路整合成的**单一、自包含插件源码树**。
原版分散在 2 个仓库、5 个包里（`luci-app-fwx-fastnat` + `luci-app-fwx-resources` + `fwxd` + `libfwx_common` + `shortcut-fe` + `fwx`），
这里合并为一个目录，一次编译产出全部组件。

## 目录结构

```
luci-app-fastnat/
├── Makefile                      # 一个 Makefile 产出 3 个包
├── README.md
├── src/
│   ├── kernel/                   # Shortcut-FE 内核引擎
│   │   ├── sfe.h  sfe_backport.h
│   │   ├── sfe_cm.c  sfe_cm.h            # 连接管理 + /sys/sfe_cm 开关
│   │   ├── sfe_ipv4.c  sfe_ipv6.c        # 快速转发 + /proc 统计
│   │   ├── sfe_fwx_compat.h              # ★新增：DPI 依赖隔离层
│   │   ├── fwx_fastpath.h                # DPI 模式下的记账接口
│   │   ├── Kconfig  Makefile
│   └── backend/                  # 独立 ubus 后端（替代 fwxd）
│       ├── fastnatd.c                    # ★新增：只含 fastnat 的 ubus 守护进程
│       ├── fwx_fastnat.c  fwx_fastnat.h  # 原版业务逻辑，逐字保留
│       ├── fwx_uci.c  fwx_uci.h          # UCI 读写辅助
│       ├── fwx_common.c  fwx.h           # 响应封装 + API 码
│       └── Makefile
├── files/
│   ├── fastnat.init              # procd 启动脚本（含 fwxd 共存判断）
│   ├── fastnat.config            # 默认 enabled=0 dpi_compat=1
│   └── luci-app-fastnat.json     # rpcd ACL
├── luasrc/
│   ├── controller/fastnat.lua
│   └── view/fastnat/fastnat.htm
├── htdocs/luci-static/resources/fastnat/
│   ├── fastnat.css               # 内置样式，不再依赖 fwx-resources
│   └── fastnat-ui.js             # 内置弹窗，不再依赖 jQuery
├── po/zh_Hans/fastnat.po
└── kernel-patches/               # ★内核补丁（见下方必读）
    ├── 952-net-netfilter-shortcut-fe-hook.patch
    └── 950-fwx-nf-conn-struct-user-hook.patch
```

## 必读：内核补丁不可省略

SFE 是**内核态转发引擎**，它挂进 `net/core/dev.c` 的收包路径，这不是任何用户态插件能自带的。
所以本包无法做到「纯 ipk 装完即用」，必须让内核带上钩子：

| 补丁 | 作用 | 是否必需 |
|---|---|---|
| `952-net-netfilter-shortcut-fe-hook.patch` | 提供 `athrs_fast_nat_recv` 收包钩子、`skb->fast_forwarded` 标记、`nf_sfe_*` 连接销毁通知、`CONFIG_SHORTCUT_FE` | **必需** |
| `950-fwx-nf-conn-struct-user-hook.patch` | 给 `struct nf_conn` 加 `fwx_data`（app_id/action），供 DPI 联动 | 仅 `SFE_FWX_DPI=1` 时必需 |

放进 OpenWrt 源码树：

```bash
cp kernel-patches/952-*.patch  target/linux/generic/hack-6.12/
# 需要 DPI 联动时再加：
cp kernel-patches/950-*.patch  target/linux/generic/hack-6.12/
```

补丁基于 **Linux 6.12**（FanchmWrt 25.12 分支）。换内核版本需要自行 rebase。

## 相比原版的改动

| 项 | 原版 | 本整合包 |
|---|---|---|
| 后端 | 依赖完整 `fwxd`（含应用过滤/DPI特征库/DDNS/无线/用户统计，约 1MB 源码） | `fastnatd`，只暴露 fastnat 三个 API |
| ubus 接口 | `fwx` 对象 → `common` 方法 → `{api,data}` | **完全一致**，前端无需改协议 |
| DPI 内核依赖 | 硬编码 `ct->fwx_data` + `fwx_account_fastpath_packet()` | 收进 `sfe_fwx_compat.h`，编译期可选 |
| 前端资源 | 依赖 `luci-app-fwx-resources`（jQuery + echarts 共 ~870KB） | 内置精简 CSS/JS，零外部依赖 |
| 菜单位置 | `admin/fwx_network/fastnat`（需 FanchmWrt 主题菜单） | `admin/network/fastnat`（标准 LuCI 网络菜单下） |
| 页面字段/交互 | 加速引擎、兼容DPI、引擎状态、连接数、包数、流量 | **完全一致**，5 秒轮询、关闭 DPI 二次确认均保留 |

`fwx_fastnat.c` 逐字保留原版，未改一行业务逻辑；`sfe_ipv4.c` / `sfe_ipv6.c` / `sfe_cm.c`
只把 4 处 DPI 记账调用和 1 处 `fwx_data` 读取替换为兼容层函数，转发逻辑不变。

## 编译

```bash
# 放入 OpenWrt 源码树
cp -r luci-app-fastnat package/

make menuconfig
#   LuCI → Applications → luci-app-fastnat
#   （自动带上 fastnatd 和 kmod-fastnat）

make package/luci-app-fastnat/compile V=s
```

产出 3 个包：`luci-app-fastnat`、`fastnatd`、`kmod-fastnat`。

需要 FanchmWrt 的 DPI 联动（应用识别/应用过滤与加速互斥）：

```bash
make package/luci-app-fastnat/compile V=s SFE_FWX_DPI=1
```

此模式要求已打 950 补丁且存在 `kmod-fwx`。

## 与 fwxd 共存

`files/fastnat.init` 会检测 `/usr/bin/fwxd`：存在则**不启动** `fastnatd`，
避免两个进程争抢同一个 `fwx` ubus 对象。也就是说在完整 FanchmWrt 固件上，
本包的前端会直接复用原有 fwxd 后端。

## 运行时校验

```bash
ubus call fwx common '{"api":"get_fastnat_status"}'
cat /sys/sfe_cm/enabled /sys/sfe_cm/dpi_compat
cat /proc/sfe_ipv4/sfe_ipv4_stat
uci show fastnat
```

页面数字来源：`num_connections` → 正在加速连接；`pkts_forwarded` → 加速数据包数；
`bytes_forwarded` → 已加速流量（前端按 1024 进制换算）。

## 已验证 / 未验证

已在真实环境验证：

- `fwx_common.c` / `fwx_uci.c` / `fwx_fastnat.c` / `fastnatd.c`
  用上游 libubox + ubus + uci + json-c 头文件 `gcc -Wall -Wextra` **零警告编译通过**，
  并成功链接为可执行文件，无未解析符号。
- `fastnat.lua` 通过 `luac -p` 语法检查。
- `fastnat.htm` 内联 JS 与 `fastnat-ui.js` 通过 `node --check`。
- `luci-app-fastnat.json` 通过 JSON 解析校验。
- `sfe_fwx_compat.h` 独立模式（不带 DPI）编译通过；DPI 模式正确要求打补丁内核。

**未验证**：三个 `.ko` 未在真实 OpenWrt 内核树中编译（需要 6.12 + 上述补丁的完整
buildroot），因此内核模块的实际编译与运行需要你在目标环境完成。

## 来源与许可

- SFE 引擎：Copyright (c) 2013-2018 The Linux Foundation，ISC
- fastnat 业务逻辑 / UCI 辅助 / 前端文案：Copyright (c) 2026 destan19(TT) www.fanchmwrt.com，GPL-2.0-or-later
- 整合层（`fastnatd.c`、`sfe_fwx_compat.h`、精简 `fwx.h`、内置前端资源、Makefile）：GPL-2.0-or-later

上游出处：
- https://github.com/fanchmwrt/fanchmwrt （分支 `fanchmwrt-25.12.6`）
- https://github.com/fanchmwrt/fanchmwrt-packages （分支 `main`）