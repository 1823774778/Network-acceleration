// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Response helper extracted verbatim from fwxd's fwx_common.c.
 * Copyright(c) 2026 destan19(TT) <www.fanchmwrt.com>
 */
#include "fwx.h"

struct json_object * fwx_gen_api_response_data(int code, struct json_object *data_obj){
    struct json_object *root_obj = json_object_new_object();
    if (!root_obj)
        return NULL;
    json_object_object_add(root_obj, "code", json_object_new_int(code));
    if (data_obj)
        json_object_object_add(root_obj, "data", data_obj);
    return root_obj;
}