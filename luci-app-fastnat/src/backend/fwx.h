// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Minimal compatibility shim for the standalone fastnat backend.
 *
 * Upstream fwx_fastnat.c is compiled inside the fwxd daemon and includes the
 * full "fwx.h".  Only two things from that header are actually used by the
 * fastnat code path: the API status codes and fwx_gen_api_response_data().
 * This header reproduces exactly those, so fwx_fastnat.c can be built
 * verbatim without dragging in the rest of fwxd.
 *
 * Copyright(c) 2026 destan19(TT) <www.fanchmwrt.com>
 */
#ifndef __FWX_H___
#define __FWX_H___

#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include "fwx_uci.h"

#define API_CODE_SUCCESS 2000
#define API_CODE_ERROR 4000

struct json_object *fwx_gen_api_response_data(int code, struct json_object *data_obj);

#endif