#
# Makefile for the standalone Shortcut FE engine.
#
# SFE_FWX_DPI=1 restores the FanchmWrt DPI integration; it requires the
# FanchmWrt kernel patches (ct->fwx_data) and kmod-fwx.  Default is off.
#

obj-m += shortcut-fe.o

ifdef SFE_SUPPORT_IPV6
obj-m += shortcut-fe-ipv6.o
endif

obj-m += shortcut-fe-cm.o

ccflags-$(SFE_SUPPORT_IPV6) += -DSFE_SUPPORT_IPV6
ccflags-y += -Wall -Werror

ifeq ($(SFE_FWX_DPI),1)
ccflags-y += -DSFE_FWX_DPI
ifdef FWX_INCLUDE_DIR
ccflags-y += -I$(FWX_INCLUDE_DIR)
endif
endif

shortcut-fe-objs := \
	sfe_ipv4.o

ifdef SFE_SUPPORT_IPV6
shortcut-fe-ipv6-objs := \
	sfe_ipv6.o
endif

shortcut-fe-cm-objs := \
	sfe_cm.o