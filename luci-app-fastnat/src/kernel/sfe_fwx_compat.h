/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sfe_fwx_compat.h - optional FanchmWrt DPI integration for SFE.
 *
 * Upstream FanchmWrt builds shortcut-fe against a patched kernel that adds
 * `struct nf_fwx_data fwx_data` to struct nf_conn, and links against kmod-fwx
 * which exports fwx_account_fastpath_packet().  Neither exists on a stock
 * OpenWrt kernel.
 *
 * Define SFE_FWX_DPI to keep the original behaviour (requires the FanchmWrt
 * kernel patches + kmod-fwx).  Leave it undefined for a standalone build:
 * the DPI accounting becomes a no-op and the app-id/drop gate always reports
 * "not blocked", which is what upstream does at runtime when dpi_compat is 0.
 *
 * Locking: the packet snapshot must be filled while si->lock is still held
 * (it reads cm->xmit_dest_mac), and the accounting call must happen after the
 * unlock.  The two helpers below are split accordingly so call sites keep the
 * exact upstream ordering.
 */
#ifndef _SFE_FWX_COMPAT_H_
#define _SFE_FWX_COMPAT_H_

#include <linux/types.h>
#include <linux/if_ether.h>

struct net_device;
struct nf_conn;

#ifdef SFE_FWX_DPI

#include <fwx_fastpath.h>

#define SFE_FWX_PACKET_DECL(name)	struct fwx_fastpath_packet name

/* Call with si->lock held. */
static inline void sfe_fwx_packet_fill(struct fwx_fastpath_packet *packet,
				       const u8 *src_mac, const u8 *dest_mac,
				       u32 len, bool is_v6)
{
	memcpy(packet->src_mac, src_mac, ETH_ALEN);
	memcpy(packet->dest_mac, dest_mac, ETH_ALEN);
	packet->bytes = len;
	packet->is_v6 = is_v6;
}

/* Call after si->lock is released. */
static inline void sfe_fwx_account(struct net_device *dev,
				   struct fwx_fastpath_packet *packet)
{
	fwx_account_fastpath_packet(dev, packet);
}

/* Reads the per-conntrack DPI verdict installed by kmod-fwx. */
static inline bool sfe_fwx_conn_blocked(struct nf_conn *ct, u32 *app_id, u8 *drop)
{
	*app_id = READ_ONCE(ct->fwx_data.app_id);
	*drop = READ_ONCE(ct->fwx_data.action);
	return true;
}

#else /* !SFE_FWX_DPI */

/* Placeholder keeps the upstream call sites compiling unchanged. */
struct sfe_fwx_packet_stub {
	u8 src_mac[ETH_ALEN];
	u8 dest_mac[ETH_ALEN];
	u32 bytes;
	bool is_v6;
};

#define SFE_FWX_PACKET_DECL(name)	struct sfe_fwx_packet_stub name

static inline void sfe_fwx_packet_fill(struct sfe_fwx_packet_stub *packet,
				       const u8 *src_mac, const u8 *dest_mac,
				       u32 len, bool is_v6)
{
	(void)packet;
	(void)src_mac;
	(void)dest_mac;
	(void)len;
	(void)is_v6;
}

static inline void sfe_fwx_account(struct net_device *dev,
				   struct sfe_fwx_packet_stub *packet)
{
	(void)dev;
	(void)packet;
}

static inline bool sfe_fwx_conn_blocked(struct nf_conn *ct, u32 *app_id, u8 *drop)
{
	(void)ct;
	*app_id = 0;
	*drop = 0;
	return false;
}

#endif /* SFE_FWX_DPI */

#endif /* _SFE_FWX_COMPAT_H_ */