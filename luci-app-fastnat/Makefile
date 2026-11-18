#
# luci-app-fastnat - self-contained network acceleration (SFE) plugin.
#
# Bundles, in one package directory:
#   * the Shortcut-FE kernel modules  (src/kernel)
#   * a standalone ubus backend       (src/backend, replaces fwxd)
#   * the LuCI controller/view/i18n   (luasrc, po)
#   * init script + default config    (files)
#
# Set SFE_FWX_DPI=1 in the environment to build against the FanchmWrt patched
# kernel with kmod-fwx present; that restores per-connection DPI accounting.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=luci-app-fastnat
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-or-later ISC
PKG_MAINTAINER:=fastnat integration

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

SFE_FWX_DPI ?= 0

define Package/luci-app-fastnat
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=LuCI network acceleration (Shortcut-FE)
  PKGARCH:=all
  DEPENDS:=+luci-compat +luci-lib-jsonc +fastnatd +kmod-fastnat
endef

define Package/luci-app-fastnat/description
  Self-contained LuCI page for NAT forwarding acceleration, bundling the
  Shortcut-FE engine, its ubus backend and the web UI in one source tree.
endef

define Package/fastnatd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Standalone fastnat ubus backend
  DEPENDS:=+libubox +libubus +libblobmsg-json +libuci +libjson-c
endef

define Package/fastnatd/description
  Minimal ubus daemon exposing the fastnat status/config API. Not needed
  when the full fwxd daemon is installed.
endef

define Package/fastnatd/conffiles
/etc/config/fastnat
endef

define KernelPackage/fastnat
  SUBMENU:=Network Support
  TITLE:=Shortcut-FE forwarding engine (standalone)
  DEPENDS:=@IPV6 +kmod-nf-conntrack
  AUTOLOAD:=$(call AutoLoad,09,shortcut-fe shortcut-fe-ipv6 shortcut-fe-cm)
  KCONFIG:= \
	CONFIG_SHORTCUT_FE=y \
	CONFIG_NF_CONNTRACK_EVENTS=y \
	CONFIG_NF_CONNTRACK_MARK=y \
	CONFIG_NF_CONNTRACK_TIMEOUT=y
  FILES:= \
	$(PKG_BUILD_DIR)/kernel/shortcut-fe.ko \
	$(PKG_BUILD_DIR)/kernel/shortcut-fe-ipv6.ko \
	$(PKG_BUILD_DIR)/kernel/shortcut-fe-cm.ko
endef

define KernelPackage/fastnat/description
  Shortcut Forwarding Engine with connection manager. Requires the SFE
  receive-hook kernel patch (see kernel-patches/).
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/. $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/backend \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		all
	$(KERNEL_MAKE) -C "$(LINUX_DIR)" \
		M="$(PKG_BUILD_DIR)/kernel" \
		SFE_SUPPORT_IPV6=y \
		SFE_FWX_DPI=$(SFE_FWX_DPI) \
		modules
endef

define Package/fastnatd/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/backend/fastnatd $(1)/usr/bin/fastnatd
	$(INSTALL_BIN) ./files/fastnat.init $(1)/etc/init.d/fastnat
	$(INSTALL_CONF) ./files/fastnat.config $(1)/etc/config/fastnat
endef

define Package/luci-app-fastnat/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/fastnat
	$(INSTALL_DIR) $(1)/www/luci-static/resources/fastnat
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./luasrc/controller/fastnat.lua $(1)/usr/lib/lua/luci/controller/
	$(INSTALL_DATA) ./luasrc/view/fastnat/fastnat.htm $(1)/usr/lib/lua/luci/view/fastnat/
	$(INSTALL_DATA) ./htdocs/luci-static/resources/fastnat/fastnat.css $(1)/www/luci-static/resources/fastnat/
	$(INSTALL_DATA) ./htdocs/luci-static/resources/fastnat/fastnat-ui.js $(1)/www/luci-static/resources/fastnat/
	$(INSTALL_DATA) ./files/luci-app-fastnat.json $(1)/usr/share/rpcd/acl.d/
endef

$(eval $(call BuildPackage,fastnatd))
$(eval $(call KernelPackage,fastnat))
$(eval $(call BuildPackage,luci-app-fastnat))