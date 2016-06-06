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

#ifndef NSC_CLIENT_PROTOCOL_H
#define NSC_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_buffer;
struct wl_nsc;

extern const struct wl_interface wl_buffer_interface;
extern const struct wl_interface wl_nsc_interface;

#ifndef WL_NSC_ERROR_ENUM
#define WL_NSC_ERROR_ENUM
enum wl_nsc_error {
	WL_NSC_ERROR_AUTHENTICATE_FAIL = 0,
};
#endif /* WL_NSC_ERROR_ENUM */

#ifndef WL_NSC_CLIENT_ENUM
#define WL_NSC_CLIENT_ENUM
enum wl_nsc_client {
	WL_NSC_CLIENT_SURFACE = 0x01,
	WL_NSC_CLIENT_SIMPLEVIDEODECODER = 0x02,
	WL_NSC_CLIENT_SIMPLEAUDIODECODER = 0x04,
	WL_NSC_CLIENT_SIMPLEAUDIOPLAYBACK = 0x08,
	WL_NSC_CLIENT_SIMPLEENCODER = 0x10,
	WL_NSC_CLIENT_AUDIOCAPTURE = 0x20,
};
#endif /* WL_NSC_CLIENT_ENUM */

struct wl_nsc_listener {
	/**
	 * standby_status - (none)
	 * @status: (none)
	 */
	void (*standby_status)(void *data,
			       struct wl_nsc *wl_nsc,
			       struct wl_array *status);
	/**
	 * connectID - (none)
	 * @id: (none)
	 */
	void (*connectID)(void *data,
			  struct wl_nsc *wl_nsc,
			  uint32_t id);
	/**
	 * composition - (none)
	 * @buffer: (none)
	 */
	void (*composition)(void *data,
			    struct wl_nsc *wl_nsc,
			    struct wl_array *buffer);
	/**
	 * authenticated - (none)
	 * @certificateData: (none)
	 * @certificateLength: (none)
	 */
	void (*authenticated)(void *data,
			      struct wl_nsc *wl_nsc,
			      const char *certificateData,
			      uint32_t certificateLength);
	/**
	 * clientID_created - (none)
	 * @clientID: (none)
	 */
	void (*clientID_created)(void *data,
				 struct wl_nsc *wl_nsc,
				 struct wl_array *clientID);
	/**
	 * display_geometry - (none)
	 * @width: (none)
	 * @height: (none)
	 */
	void (*display_geometry)(void *data,
				 struct wl_nsc *wl_nsc,
				 uint32_t width,
				 uint32_t height);
	/**
	 * audiosettings - (none)
	 * @buffer: (none)
	 */
	void (*audiosettings)(void *data,
			      struct wl_nsc *wl_nsc,
			      struct wl_array *buffer);
	/**
	 * displaystatus - (none)
	 * @buffer: (none)
	 */
	void (*displaystatus)(void *data,
			      struct wl_nsc *wl_nsc,
			      struct wl_array *buffer);
	/**
	 * picturequalitysettings - (none)
	 * @buffer: (none)
	 */
	void (*picturequalitysettings)(void *data,
				       struct wl_nsc *wl_nsc,
				       struct wl_array *buffer);
	/**
	 * displaysettings - (none)
	 * @buffer: (none)
	 */
	void (*displaysettings)(void *data,
				struct wl_nsc *wl_nsc,
				struct wl_array *buffer);
};

static inline int
wl_nsc_add_listener(struct wl_nsc *wl_nsc,
		    const struct wl_nsc_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_nsc,
				     (void (**)(void)) listener, data);
}

#define WL_NSC_AUTHENTICATE	0
#define WL_NSC_NXCLIENT_ALLOC	1
#define WL_NSC_REQUEST_CLIENTID	2
#define WL_NSC_CREATE_WINDOW	3
#define WL_NSC_CREATE_BUFFER	4
#define WL_NSC_NXCLIENT_GET_SURFACECLIENT_COMPOSITION	5
#define WL_NSC_NXCLIENT_CONNECT	6
#define WL_NSC_ACK_STANDBY	7
#define WL_NSC_GET_STANDBY_STATUS	8
#define WL_NSC_GET_DISPLAY_GEOMETRY	9
#define WL_NSC_NXCLIENT_GET_AUDIOSETTINGS	10
#define WL_NSC_NXCLIENT_SET_AUDIOSETTINGS	11
#define WL_NSC_NXCLIENT_GET_DISPLAYSTATUS	12
#define WL_NSC_NXCLIENT_GET_PICTUREQUALITYSETTINGS	13
#define WL_NSC_NXCLIENT_GET_DISPLAYSETTINGS	14

static inline void
wl_nsc_set_user_data(struct wl_nsc *wl_nsc, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_nsc, user_data);
}

static inline void *
wl_nsc_get_user_data(struct wl_nsc *wl_nsc)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_nsc);
}

static inline void
wl_nsc_destroy(struct wl_nsc *wl_nsc)
{
	wl_proxy_destroy((struct wl_proxy *) wl_nsc);
}

static inline void
wl_nsc_authenticate(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_AUTHENTICATE);
}

static inline void
wl_nsc_nxclient_alloc(struct wl_nsc *wl_nsc, struct wl_array *settings)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_ALLOC, settings);
}

static inline void
wl_nsc_request_clientID(struct wl_nsc *wl_nsc, uint32_t clientType)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_REQUEST_CLIENTID, clientType);
}

static inline void
wl_nsc_create_window(struct wl_nsc *wl_nsc, uint32_t clientID, uint32_t surfaceClientHandle, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_CREATE_WINDOW, clientID, surfaceClientHandle, width, height);
}

static inline struct wl_buffer *
wl_nsc_create_buffer(struct wl_nsc *wl_nsc, uint32_t surfaceClientID, uint32_t width, uint32_t height)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_nsc,
			 WL_NSC_CREATE_BUFFER, &wl_buffer_interface, NULL, surfaceClientID, width, height);

	return (struct wl_buffer *) id;
}

static inline void
wl_nsc_nxclient_get_surfaceclient_composition(struct wl_nsc *wl_nsc, int32_t clientID)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_GET_SURFACECLIENT_COMPOSITION, clientID);
}

static inline void
wl_nsc_nxclient_connect(struct wl_nsc *wl_nsc, struct wl_array *settings, uint32_t clientType)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_CONNECT, settings, clientType);
}

static inline void
wl_nsc_ack_standby(struct wl_nsc *wl_nsc, uint32_t done)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_ACK_STANDBY, done);
}

static inline void
wl_nsc_get_standby_status(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_GET_STANDBY_STATUS);
}

static inline void
wl_nsc_get_display_geometry(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_GET_DISPLAY_GEOMETRY);
}

static inline void
wl_nsc_nxclient_get_audiosettings(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_GET_AUDIOSETTINGS);
}

static inline void
wl_nsc_nxclient_set_audiosettings(struct wl_nsc *wl_nsc, struct wl_array *settings)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_SET_AUDIOSETTINGS, settings);
}

static inline void
wl_nsc_nxclient_get_displaystatus(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_GET_DISPLAYSTATUS);
}

static inline void
wl_nsc_nxclient_get_picturequalitysettings(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_GET_PICTUREQUALITYSETTINGS);
}

static inline void
wl_nsc_nxclient_get_displaysettings(struct wl_nsc *wl_nsc)
{
	wl_proxy_marshal((struct wl_proxy *) wl_nsc,
			 WL_NSC_NXCLIENT_GET_DISPLAYSETTINGS);
}

#ifdef  __cplusplus
}
#endif

#endif
