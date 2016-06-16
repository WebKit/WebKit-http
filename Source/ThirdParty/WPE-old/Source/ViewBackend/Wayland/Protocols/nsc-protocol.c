/* 
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2010-2011 Intel Corporation
 * Copyright © 2014 Broadcom Corporation
 * 
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that\n the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_buffer_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
};

static const struct wl_message wl_nsc_requests[] = {
	{ "authenticate", "", types + 0 },
	{ "nxclient_alloc", "a", types + 0 },
	{ "request_clientID", "u", types + 0 },
	{ "create_window", "uuuu", types + 0 },
	{ "create_buffer", "nuuu", types + 4 },
	{ "nxclient_get_surfaceclient_composition", "i", types + 0 },
	{ "nxclient_connect", "au", types + 0 },
	{ "ack_standby", "u", types + 0 },
	{ "get_standby_status", "", types + 0 },
	{ "get_display_geometry", "", types + 0 },
	{ "nxclient_get_audiosettings", "", types + 0 },
	{ "nxclient_set_audiosettings", "a", types + 0 },
	{ "nxclient_get_displaystatus", "", types + 0 },
	{ "nxclient_get_picturequalitysettings", "", types + 0 },
	{ "nxclient_get_displaysettings", "", types + 0 },
};

static const struct wl_message wl_nsc_events[] = {
	{ "standby_status", "a", types + 0 },
	{ "connectID", "u", types + 0 },
	{ "composition", "a", types + 0 },
	{ "authenticated", "su", types + 0 },
	{ "clientID_created", "a", types + 0 },
	{ "display_geometry", "uu", types + 0 },
	{ "audiosettings", "a", types + 0 },
	{ "displaystatus", "a", types + 0 },
	{ "picturequalitysettings", "a", types + 0 },
	{ "displaysettings", "a", types + 0 },
};

WL_EXPORT const struct wl_interface wl_nsc_interface = {
	"wl_nsc", 1,
	15, wl_nsc_requests,
	10, wl_nsc_events,
};

