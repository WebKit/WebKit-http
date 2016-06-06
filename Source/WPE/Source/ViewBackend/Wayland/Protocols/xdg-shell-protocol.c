/* 
 * Copyright © 2008-2013 Kristian Høgsberg
 * Copyright © 2013      Rafael Antognolli
 * Copyright © 2013      Jasper St. Pierre
 * Copyright © 2010-2013 Intel Corporation
 * 
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
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

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_popup_interface;
extern const struct wl_interface xdg_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	&xdg_surface_interface,
	&wl_surface_interface,
	&xdg_popup_interface,
	&wl_surface_interface,
	&wl_surface_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&xdg_surface_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&wl_seat_interface,
	NULL,
	&wl_seat_interface,
	NULL,
	NULL,
	&wl_output_interface,
};

static const struct wl_message xdg_shell_requests[] = {
	{ "destroy", "", types + 0 },
	{ "use_unstable_version", "i", types + 0 },
	{ "get_xdg_surface", "no", types + 4 },
	{ "get_xdg_popup", "nooouii", types + 6 },
	{ "pong", "u", types + 0 },
};

static const struct wl_message xdg_shell_events[] = {
	{ "ping", "u", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_shell_interface = {
	"xdg_shell", 1,
	5, xdg_shell_requests,
	1, xdg_shell_events,
};

static const struct wl_message xdg_surface_requests[] = {
	{ "destroy", "", types + 0 },
	{ "set_parent", "?o", types + 13 },
	{ "set_title", "s", types + 0 },
	{ "set_app_id", "s", types + 0 },
	{ "show_window_menu", "ouii", types + 14 },
	{ "move", "ou", types + 18 },
	{ "resize", "ouu", types + 20 },
	{ "ack_configure", "u", types + 0 },
	{ "set_window_geometry", "iiii", types + 0 },
	{ "set_maximized", "", types + 0 },
	{ "unset_maximized", "", types + 0 },
	{ "set_fullscreen", "?o", types + 23 },
	{ "unset_fullscreen", "", types + 0 },
	{ "set_minimized", "", types + 0 },
};

static const struct wl_message xdg_surface_events[] = {
	{ "configure", "iiau", types + 0 },
	{ "close", "", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 1,
	14, xdg_surface_requests,
	2, xdg_surface_events,
};

static const struct wl_message xdg_popup_requests[] = {
	{ "destroy", "", types + 0 },
};

static const struct wl_message xdg_popup_events[] = {
	{ "popup_done", "", types + 0 },
};

WL_EXPORT const struct wl_interface xdg_popup_interface = {
	"xdg_popup", 1,
	1, xdg_popup_requests,
	1, xdg_popup_events,
};

