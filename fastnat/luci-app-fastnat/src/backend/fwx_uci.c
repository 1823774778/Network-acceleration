// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * UCI helpers used by the standalone fastnat backend.
 * Extracted from libfwx_common (fwx_uci.c); only the routines the fastnat
 * code path needs are kept.  Behaviour is unchanged from upstream.
 *
 * Copyright(c) 2026 destan19(TT) <www.fanchmwrt.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uci.h>
#include "fwx_uci.h"

int fwx_uci_get_int_value(struct uci_context *ctx, char *key)
{
    struct uci_element *e;
    struct uci_ptr ptr;
    int ret = -1;
    char param_tmp[128] = {0};

    strcpy(param_tmp, key);
    if (uci_lookup_ptr(ctx, &ptr, param_tmp, true) != UCI_OK)
        return ret;

    if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
        ctx->err = UCI_ERR_NOTFOUND;
        goto done;
    }

    e = ptr.last;
    switch (e->type) {
        case UCI_TYPE_SECTION:
            ret = -1;
            goto done;
        case UCI_TYPE_OPTION:
            ret = atoi(ptr.o->v.string);
            goto done;
        default:
            break;
    }
done:
    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return ret;
}

int fwx_uci_get_value(struct uci_context *ctx, char *key, char *output, int out_len)
{
    struct uci_element *e;
    struct uci_ptr ptr;
    int ret = UCI_OK;
    char param_tmp[128] = {0};

    strcpy(param_tmp, key);
    if (uci_lookup_ptr(ctx, &ptr, param_tmp, true) != UCI_OK) {
        ret = 1;
        return ret;
    }

    if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
        ctx->err = UCI_ERR_NOTFOUND;
        ret = 1;
        goto done;
    }

    e = ptr.last;
    switch (e->type) {
        case UCI_TYPE_SECTION:
            snprintf(output, out_len, "%s", ptr.s->type);
            break;
        case UCI_TYPE_OPTION:
            snprintf(output, out_len, "%s", ptr.o->v.string);
            break;
        default:
            ret = 1;
            break;
    }
done:
    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return ret;
}

int fwx_uci_set_value(struct uci_context *ctx, char *key, char *value)
{
    struct uci_ptr ptr;
    int ret = UCI_OK;
    char param_tmp[2048] = {0};

    sprintf(param_tmp, "%s=%s", key, value);
    if (uci_lookup_ptr(ctx, &ptr, param_tmp, true) != UCI_OK) {
        ret = 1;
        return ret;
    }

    ret = uci_set(ctx, &ptr);
    if (ret == UCI_OK)
        ret = uci_save(ctx, ptr.p);

    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return ret;
}

int fwx_uci_set_int_value(struct uci_context *ctx, char *key, int value)
{
    struct uci_ptr ptr;
    int ret = UCI_OK;
    char param_tmp[128] = {0};

    sprintf(param_tmp, "%s=%d", key, value);
    if (uci_lookup_ptr(ctx, &ptr, param_tmp, true) != UCI_OK) {
        ret = 1;
        return ret;
    }
    ret = uci_set(ctx, &ptr);
    if (ret == UCI_OK)
        ret = uci_save(ctx, ptr.p);

    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return ret;
}

int fwx_uci_delete(struct uci_context *ctx, char *key)
{
    struct uci_ptr ptr;
    int ret = UCI_OK;
    char param_tmp[128] = {0};

    strcpy(param_tmp, key);
    if (uci_lookup_ptr(ctx, &ptr, param_tmp, true) != UCI_OK) {
        ret = 1;
        return ret;
    }
    ret = uci_delete(ctx, &ptr);
    if (ret == UCI_OK)
        ret = uci_save(ctx, ptr.p);

    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return ret;
}

int fwx_uci_commit(struct uci_context *ctx, const char *package)
{
    struct uci_ptr ptr;

    if (!package)
        return -1;
    if (uci_lookup_ptr(ctx, &ptr, (char *)package, true) != UCI_OK)
        return -1;
    if (uci_commit(ctx, &ptr.p, false) != UCI_OK) {
        if (ptr.p)
            uci_unload(ctx, ptr.p);
        return -1;
    }
    if (ptr.p)
        uci_unload(ctx, ptr.p);
    return UCI_OK;
}