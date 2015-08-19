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

#ifndef XDG_SHELL_CLIENT_PROTOCOL_H
#define XDG_SHELL_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_output;
struct wl_seat;
struct wl_surface;
struct xdg_popup;
struct xdg_shell;
struct xdg_surface;

extern const struct wl_interface xdg_shell_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_popup_interface;

#ifndef XDG_SHELL_VERSION_ENUM
#define XDG_SHELL_VERSION_ENUM
/**
 * xdg_shell_version - latest protocol version
 * @XDG_SHELL_VERSION_CURRENT: Always the latest version
 *
 * The 'current' member of this enum gives the version of the protocol.
 * Implementations can compare this to the version they implement using
 * static_assert to ensure the protocol and implementation versions match.
 */
enum xdg_shell_version {
	XDG_SHELL_VERSION_CURRENT = 5,
};
#endif /* XDG_SHELL_VERSION_ENUM */

#ifndef XDG_SHELL_ERROR_ENUM
#define XDG_SHELL_ERROR_ENUM
enum xdg_shell_error {
	XDG_SHELL_ERROR_ROLE = 0,
	XDG_SHELL_ERROR_DEFUNCT_SURFACES = 1,
	XDG_SHELL_ERROR_NOT_THE_TOPMOST_POPUP = 2,
	XDG_SHELL_ERROR_INVALID_POPUP_PARENT = 3,
};
#endif /* XDG_SHELL_ERROR_ENUM */

/**
 * xdg_shell - create desktop-style surfaces
 * @ping: check if the client is alive
 *
 * xdg_shell allows clients to turn a wl_surface into a "real window"
 * which can be dragged, resized, stacked, and moved around by the user.
 * Everything about this interface is suited towards traditional desktop
 * environments.
 */
struct xdg_shell_listener {
	/**
	 * ping - check if the client is alive
	 * @serial: pass this to the pong request
	 *
	 * The ping event asks the client if it's still alive. Pass the
	 * serial specified in the event back to the compositor by sending
	 * a "pong" request back with the specified serial.
	 *
	 * Compositors can use this to determine if the client is still
	 * alive. It's unspecified what will happen if the client doesn't
	 * respond to the ping request, or in what timeframe. Clients
	 * should try to respond in a reasonable amount of time.
	 *
	 * A compositor is free to ping in any way it wants, but a client
	 * must always respond to any xdg_shell object it created.
	 */
	void (*ping)(void *data,
		     struct xdg_shell *xdg_shell,
		     uint32_t serial);
};

static inline int
xdg_shell_add_listener(struct xdg_shell *xdg_shell,
		       const struct xdg_shell_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_shell,
				     (void (**)(void)) listener, data);
}

#define XDG_SHELL_DESTROY	0
#define XDG_SHELL_USE_UNSTABLE_VERSION	1
#define XDG_SHELL_GET_XDG_SURFACE	2
#define XDG_SHELL_GET_XDG_POPUP	3
#define XDG_SHELL_PONG	4

static inline void
xdg_shell_set_user_data(struct xdg_shell *xdg_shell, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) xdg_shell, user_data);
}

static inline void *
xdg_shell_get_user_data(struct xdg_shell *xdg_shell)
{
	return wl_proxy_get_user_data((struct wl_proxy *) xdg_shell);
}

static inline void
xdg_shell_destroy(struct xdg_shell *xdg_shell)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_shell,
			 XDG_SHELL_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) xdg_shell);
}

static inline void
xdg_shell_use_unstable_version(struct xdg_shell *xdg_shell, int32_t version)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_shell,
			 XDG_SHELL_USE_UNSTABLE_VERSION, version);
}

static inline struct xdg_surface *
xdg_shell_get_xdg_surface(struct xdg_shell *xdg_shell, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) xdg_shell,
			 XDG_SHELL_GET_XDG_SURFACE, &xdg_surface_interface, NULL, surface);

	return (struct xdg_surface *) id;
}

static inline struct xdg_popup *
xdg_shell_get_xdg_popup(struct xdg_shell *xdg_shell, struct wl_surface *surface, struct wl_surface *parent, struct wl_seat *seat, uint32_t serial, int32_t x, int32_t y)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) xdg_shell,
			 XDG_SHELL_GET_XDG_POPUP, &xdg_popup_interface, NULL, surface, parent, seat, serial, x, y);

	return (struct xdg_popup *) id;
}

static inline void
xdg_shell_pong(struct xdg_shell *xdg_shell, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_shell,
			 XDG_SHELL_PONG, serial);
}

#ifndef XDG_SURFACE_RESIZE_EDGE_ENUM
#define XDG_SURFACE_RESIZE_EDGE_ENUM
/**
 * xdg_surface_resize_edge - edge values for resizing
 * @XDG_SURFACE_RESIZE_EDGE_NONE: (none)
 * @XDG_SURFACE_RESIZE_EDGE_TOP: (none)
 * @XDG_SURFACE_RESIZE_EDGE_BOTTOM: (none)
 * @XDG_SURFACE_RESIZE_EDGE_LEFT: (none)
 * @XDG_SURFACE_RESIZE_EDGE_TOP_LEFT: (none)
 * @XDG_SURFACE_RESIZE_EDGE_BOTTOM_LEFT: (none)
 * @XDG_SURFACE_RESIZE_EDGE_RIGHT: (none)
 * @XDG_SURFACE_RESIZE_EDGE_TOP_RIGHT: (none)
 * @XDG_SURFACE_RESIZE_EDGE_BOTTOM_RIGHT: (none)
 *
 * These values are used to indicate which edge of a surface is being
 * dragged in a resize operation. The server may use this information to
 * adapt its behavior, e.g. choose an appropriate cursor image.
 */
enum xdg_surface_resize_edge {
	XDG_SURFACE_RESIZE_EDGE_NONE = 0,
	XDG_SURFACE_RESIZE_EDGE_TOP = 1,
	XDG_SURFACE_RESIZE_EDGE_BOTTOM = 2,
	XDG_SURFACE_RESIZE_EDGE_LEFT = 4,
	XDG_SURFACE_RESIZE_EDGE_TOP_LEFT = 5,
	XDG_SURFACE_RESIZE_EDGE_BOTTOM_LEFT = 6,
	XDG_SURFACE_RESIZE_EDGE_RIGHT = 8,
	XDG_SURFACE_RESIZE_EDGE_TOP_RIGHT = 9,
	XDG_SURFACE_RESIZE_EDGE_BOTTOM_RIGHT = 10,
};
#endif /* XDG_SURFACE_RESIZE_EDGE_ENUM */

#ifndef XDG_SURFACE_STATE_ENUM
#define XDG_SURFACE_STATE_ENUM
/**
 * xdg_surface_state - types of state on the surface
 * @XDG_SURFACE_STATE_MAXIMIZED: the surface is maximized
 * @XDG_SURFACE_STATE_FULLSCREEN: the surface is fullscreen
 * @XDG_SURFACE_STATE_RESIZING: (none)
 * @XDG_SURFACE_STATE_ACTIVATED: (none)
 *
 * The different state values used on the surface. This is designed for
 * state values like maximized, fullscreen. It is paired with the configure
 * event to ensure that both the client and the compositor setting the
 * state can be synchronized.
 *
 * States set in this way are double-buffered. They will get applied on the
 * next commit.
 *
 * Desktop environments may extend this enum by taking up a range of values
 * and documenting the range they chose in this description. They are not
 * required to document the values for the range that they chose. Ideally,
 * any good extensions from a desktop environment should make its way into
 * standardization into this enum.
 *
 * The current reserved ranges are:
 *
 * 0x0000 - 0x0FFF: xdg-shell core values, documented below. 0x1000 -
 * 0x1FFF: GNOME
 */
enum xdg_surface_state {
	XDG_SURFACE_STATE_MAXIMIZED = 1,
	XDG_SURFACE_STATE_FULLSCREEN = 2,
	XDG_SURFACE_STATE_RESIZING = 3,
	XDG_SURFACE_STATE_ACTIVATED = 4,
};
#endif /* XDG_SURFACE_STATE_ENUM */

/**
 * xdg_surface - A desktop window
 * @configure: suggest a surface change
 * @close: surface wants to be closed
 *
 * An interface that may be implemented by a wl_surface, for
 * implementations that provide a desktop-style user interface.
 *
 * It provides requests to treat surfaces like windows, allowing to set
 * properties like maximized, fullscreen, minimized, and to move and resize
 * them, and associate metadata like title and app id.
 *
 * The client must call wl_surface.commit on the corresponding wl_surface
 * for the xdg_surface state to take effect. Prior to committing the new
 * state, it can set up initial configuration, such as maximizing or
 * setting a window geometry.
 *
 * Even without attaching a buffer the compositor must respond to initial
 * committed configuration, for instance sending a configure event with
 * expected window geometry if the client maximized its surface during
 * initialization.
 *
 * For a surface to be mapped by the compositor the client must have
 * committed both an xdg_surface state and a buffer.
 */
struct xdg_surface_listener {
	/**
	 * configure - suggest a surface change
	 * @width: (none)
	 * @height: (none)
	 * @states: (none)
	 * @serial: (none)
	 *
	 * The configure event asks the client to resize its surface or
	 * to change its state.
	 *
	 * The width and height arguments specify a hint to the window
	 * about how its surface should be resized in window geometry
	 * coordinates. See set_window_geometry.
	 *
	 * If the width or height arguments are zero, it means the client
	 * should decide its own window dimension. This may happen when the
	 * compositor need to configure the state of the surface but
	 * doesn't have any information about any previous or expected
	 * dimension.
	 *
	 * The states listed in the event specify how the width/height
	 * arguments should be interpreted, and possibly how it should be
	 * drawn.
	 *
	 * Clients should arrange their surface for the new size and
	 * states, and then send a ack_configure request with the serial
	 * sent in this configure event at some point before committing the
	 * new surface.
	 *
	 * If the client receives multiple configure events before it can
	 * respond to one, it is free to discard all but the last event it
	 * received.
	 */
	void (*configure)(void *data,
			  struct xdg_surface *xdg_surface,
			  int32_t width,
			  int32_t height,
			  struct wl_array *states,
			  uint32_t serial);
	/**
	 * close - surface wants to be closed
	 *
	 * The close event is sent by the compositor when the user wants
	 * the surface to be closed. This should be equivalent to the user
	 * clicking the close button in client-side decorations, if your
	 * application has any...
	 *
	 * This is only a request that the user intends to close your
	 * window. The client may choose to ignore this request, or show a
	 * dialog to ask the user to save their data...
	 */
	void (*close)(void *data,
		      struct xdg_surface *xdg_surface);
};

static inline int
xdg_surface_add_listener(struct xdg_surface *xdg_surface,
			 const struct xdg_surface_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_surface,
				     (void (**)(void)) listener, data);
}

#define XDG_SURFACE_DESTROY	0
#define XDG_SURFACE_SET_PARENT	1
#define XDG_SURFACE_SET_TITLE	2
#define XDG_SURFACE_SET_APP_ID	3
#define XDG_SURFACE_SHOW_WINDOW_MENU	4
#define XDG_SURFACE_MOVE	5
#define XDG_SURFACE_RESIZE	6
#define XDG_SURFACE_ACK_CONFIGURE	7
#define XDG_SURFACE_SET_WINDOW_GEOMETRY	8
#define XDG_SURFACE_SET_MAXIMIZED	9
#define XDG_SURFACE_UNSET_MAXIMIZED	10
#define XDG_SURFACE_SET_FULLSCREEN	11
#define XDG_SURFACE_UNSET_FULLSCREEN	12
#define XDG_SURFACE_SET_MINIMIZED	13

static inline void
xdg_surface_set_user_data(struct xdg_surface *xdg_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) xdg_surface, user_data);
}

static inline void *
xdg_surface_get_user_data(struct xdg_surface *xdg_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) xdg_surface);
}

static inline void
xdg_surface_destroy(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) xdg_surface);
}

static inline void
xdg_surface_set_parent(struct xdg_surface *xdg_surface, struct xdg_surface *parent)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_PARENT, parent);
}

static inline void
xdg_surface_set_title(struct xdg_surface *xdg_surface, const char *title)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_TITLE, title);
}

static inline void
xdg_surface_set_app_id(struct xdg_surface *xdg_surface, const char *app_id)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_APP_ID, app_id);
}

static inline void
xdg_surface_show_window_menu(struct xdg_surface *xdg_surface, struct wl_seat *seat, uint32_t serial, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SHOW_WINDOW_MENU, seat, serial, x, y);
}

static inline void
xdg_surface_move(struct xdg_surface *xdg_surface, struct wl_seat *seat, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_MOVE, seat, serial);
}

static inline void
xdg_surface_resize(struct xdg_surface *xdg_surface, struct wl_seat *seat, uint32_t serial, uint32_t edges)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_RESIZE, seat, serial, edges);
}

static inline void
xdg_surface_ack_configure(struct xdg_surface *xdg_surface, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_ACK_CONFIGURE, serial);
}

static inline void
xdg_surface_set_window_geometry(struct xdg_surface *xdg_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_WINDOW_GEOMETRY, x, y, width, height);
}

static inline void
xdg_surface_set_maximized(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_MAXIMIZED);
}

static inline void
xdg_surface_unset_maximized(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_UNSET_MAXIMIZED);
}

static inline void
xdg_surface_set_fullscreen(struct xdg_surface *xdg_surface, struct wl_output *output)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_FULLSCREEN, output);
}

static inline void
xdg_surface_unset_fullscreen(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_UNSET_FULLSCREEN);
}

static inline void
xdg_surface_set_minimized(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_surface,
			 XDG_SURFACE_SET_MINIMIZED);
}

/**
 * xdg_popup - short-lived, popup surfaces for menus
 * @popup_done: popup interaction is done
 *
 * A popup surface is a short-lived, temporary surface that can be used
 * to implement menus. It takes an explicit grab on the surface that will
 * be dismissed when the user dismisses the popup. This can be done by the
 * user clicking outside the surface, using the keyboard, or even locking
 * the screen through closing the lid or a timeout.
 *
 * When the popup is dismissed, a popup_done event will be sent out, and at
 * the same time the surface will be unmapped. The xdg_popup object is now
 * inert and cannot be reactivated, so clients should destroy it.
 * Explicitly destroying the xdg_popup object will also dismiss the popup
 * and unmap the surface.
 *
 * Clients will receive events for all their surfaces during this grab
 * (which is an "owner-events" grab in X11 parlance). This is done so that
 * users can navigate through submenus and other "nested" popup windows
 * without having to dismiss the topmost popup.
 *
 * Clients that want to dismiss the popup when another surface of their own
 * is clicked should dismiss the popup using the destroy request.
 *
 * The parent surface must have either an xdg_surface or xdg_popup role.
 *
 * Specifying an xdg_popup for the parent means that the popups are nested,
 * with this popup now being the topmost popup. Nested popups must be
 * destroyed in the reverse order they were created in, e.g. the only popup
 * you are allowed to destroy at all times is the topmost one.
 *
 * If there is an existing popup when creating a new popup, the parent must
 * be the current topmost popup.
 *
 * A parent surface must be mapped before the new popup is mapped.
 *
 * When compositors choose to dismiss a popup, they will likely dismiss
 * every nested popup as well. When a compositor dismisses popups, it will
 * follow the same dismissing order as required from the client.
 *
 * The x and y arguments passed when creating the popup object specify
 * where the top left of the popup should be placed, relative to the local
 * surface coordinates of the parent surface. See xdg_shell.get_xdg_popup.
 *
 * The client must call wl_surface.commit on the corresponding wl_surface
 * for the xdg_popup state to take effect.
 *
 * For a surface to be mapped by the compositor the client must have
 * committed both the xdg_popup state and a buffer.
 */
struct xdg_popup_listener {
	/**
	 * popup_done - popup interaction is done
	 *
	 * The popup_done event is sent out when a popup is dismissed by
	 * the compositor. The client should destroy the xdg_popup object
	 * at this point.
	 */
	void (*popup_done)(void *data,
			   struct xdg_popup *xdg_popup);
};

static inline int
xdg_popup_add_listener(struct xdg_popup *xdg_popup,
		       const struct xdg_popup_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_popup,
				     (void (**)(void)) listener, data);
}

#define XDG_POPUP_DESTROY	0

static inline void
xdg_popup_set_user_data(struct xdg_popup *xdg_popup, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) xdg_popup, user_data);
}

static inline void *
xdg_popup_get_user_data(struct xdg_popup *xdg_popup)
{
	return wl_proxy_get_user_data((struct wl_proxy *) xdg_popup);
}

static inline void
xdg_popup_destroy(struct xdg_popup *xdg_popup)
{
	wl_proxy_marshal((struct wl_proxy *) xdg_popup,
			 XDG_POPUP_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) xdg_popup);
}

#ifdef  __cplusplus
}
#endif

#endif
