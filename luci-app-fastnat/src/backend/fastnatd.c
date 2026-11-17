// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * fastnatd - standalone ubus backend for the LuCI fastnat page.
 *
 * Upstream the fastnat API lives inside the fwxd daemon, which also carries
 * app filtering, DPI feature updates, DDNS, wireless, user tracking, etc.
 * This daemon exposes ONLY the fastnat methods, under the same ubus object
 * name ("fwx") and method name ("common") with the same {api, data} envelope,
 * so the unmodified LuCI controller keeps working.
 *
 * If fwxd is already installed, do not run this daemon: they would both try
 * to own the "fwx" ubus object.  The init script checks for that.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include <libubox/uloop.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <json-c/json.h>

#include "fwx.h"
#include "fwx_fastnat.h"

#define FASTNATD_UBUS_OBJECT "fwx"
#define FASTNATD_UBUS_SOCK "/var/run/ubus/ubus.sock"

static struct ubus_context *ubus_ctx;
static struct ubus_auto_conn auto_conn;
static volatile sig_atomic_t stop_requested;

struct fastnat_api {
	const char *name;
	struct json_object *(*handler)(struct json_object *req);
	int needs_data;
};

static const struct fastnat_api fastnat_apis[] = {
	{ "get_fastnat_status", fwx_api_get_fastnat_status, 0 },
	{ "get_fastnat_config", fwx_api_get_fastnat_config, 0 },
	{ "set_fastnat_config", fwx_api_set_fastnat_config, 1 },
};

enum {
	COMMON_API,
	COMMON_DATA,
	__COMMON_MAX
};

static const struct blobmsg_policy common_policy[__COMMON_MAX] = {
	[COMMON_API]  = { .name = "api",  .type = BLOBMSG_TYPE_STRING },
	[COMMON_DATA] = { .name = "data", .type = BLOBMSG_TYPE_TABLE },
};

static void fastnat_reply(struct ubus_context *ctx, struct ubus_request_data *req,
			  struct json_object *resp)
{
	struct blob_buf b = { 0 };
	char *text;

	if (!resp)
		resp = fwx_gen_api_response_data(API_CODE_ERROR, NULL);
	if (!resp) {
		ubus_send_reply(ctx, req, NULL);
		return;
	}

	text = (char *)json_object_to_json_string_ext(resp, JSON_C_TO_STRING_PLAIN);
	blob_buf_init(&b, 0);
	if (!blobmsg_add_json_from_string(&b, text)) {
		blob_buf_free(&b);
		blob_buf_init(&b, 0);
		blobmsg_add_u32(&b, "code", API_CODE_ERROR);
	}
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);
	json_object_put(resp);
}

static int ubus_handle_common(struct ubus_context *ctx, struct ubus_object *obj,
			      struct ubus_request_data *req, const char *method,
			      struct blob_attr *msg)
{
	struct blob_attr *tb[__COMMON_MAX];
	struct json_object *data = NULL, *resp = NULL;
	const char *api;
	size_t i;

	blobmsg_parse(common_policy, __COMMON_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[COMMON_API]) {
		resp = fwx_gen_api_response_data(API_CODE_ERROR, NULL);
		if (resp)
			json_object_object_add(resp, "message",
				json_object_new_string("Invalid switch value"));
		fastnat_reply(ctx, req, resp);
		return 0;
	}

	api = blobmsg_get_string(tb[COMMON_API]);
	for (i = 0; i < sizeof(fastnat_apis) / sizeof(fastnat_apis[0]); i++) {
		const struct fastnat_api *entry = &fastnat_apis[i];

		if (strcmp(api, entry->name) != 0)
			continue;

		if (entry->needs_data) {
			char *text;

			if (!tb[COMMON_DATA]) {
				resp = fwx_gen_api_response_data(API_CODE_ERROR, NULL);
				if (resp)
					json_object_object_add(resp, "message",
						json_object_new_string("Invalid switch value"));
				fastnat_reply(ctx, req, resp);
				return 0;
			}
			text = blobmsg_format_json(tb[COMMON_DATA], true);
			if (text) {
				data = json_tokener_parse(text);
				free(text);
			}
		}

		resp = entry->handler(data);
		if (data)
			json_object_put(data);
		fastnat_reply(ctx, req, resp);
		return 0;
	}

	resp = fwx_gen_api_response_data(API_CODE_ERROR, NULL);
	if (resp)
		json_object_object_add(resp, "message",
			json_object_new_string("Fastnat service is unavailable"));
	fastnat_reply(ctx, req, resp);
	return 0;
}

static struct ubus_method fastnat_methods[] = {
	UBUS_METHOD("common", ubus_handle_common, common_policy),
};

static struct ubus_object_type fastnat_object_type =
	UBUS_OBJECT_TYPE(FASTNATD_UBUS_OBJECT, fastnat_methods);

static struct ubus_object fastnat_object = {
	.name = FASTNATD_UBUS_OBJECT,
	.type = &fastnat_object_type,
	.methods = fastnat_methods,
	.n_methods = sizeof(fastnat_methods) / sizeof(fastnat_methods[0]),
};

static void fastnat_add_object(void)
{
	int ret = ubus_add_object(ubus_ctx, &fastnat_object);

	if (ret)
		syslog(LOG_ERR, "fastnatd: failed to publish ubus object: %s",
		       ubus_strerror(ret));
}

static void fastnat_conn_cb(struct ubus_context *ctx)
{
	ubus_ctx = ctx;
	fastnat_add_object();
}

static void fastnat_signal(int signo)
{
	stop_requested = 1;
	uloop_end();
}

int main(int argc, char **argv)
{
	openlog("fastnatd", LOG_PID, LOG_DAEMON);
	uloop_init();

	signal(SIGTERM, fastnat_signal);
	signal(SIGINT, fastnat_signal);
	signal(SIGPIPE, SIG_IGN);

	auto_conn.cb = fastnat_conn_cb;
	auto_conn.path = FASTNATD_UBUS_SOCK;
	ubus_auto_connect(&auto_conn);

	uloop_run();

	ubus_auto_shutdown(&auto_conn);
	uloop_done();
	closelog();
	return 0;
}