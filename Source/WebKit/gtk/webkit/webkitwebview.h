/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef webkitwebview_h
#define webkitwebview_h

#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <JavaScriptCore/JSBase.h>

#include <webkit/webkitdefines.h>
#include <webkit/webkitdom.h>
#include <webkit/webkitwebbackforwardlist.h>
#include <webkit/webkitwebframe.h>
#include <webkit/webkitwebhistoryitem.h>
#include <webkit/webkitwebsettings.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_WEB_VIEW            (webkit_web_view_get_type())
#define WEBKIT_WEB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_WEB_VIEW, WebKitWebView))
#define WEBKIT_WEB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_WEB_VIEW, WebKitWebViewClass))
#define WEBKIT_IS_WEB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_WEB_VIEW))
#define WEBKIT_IS_WEB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_WEB_VIEW))
#define WEBKIT_WEB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_WEB_VIEW, WebKitWebViewClass))

typedef struct _WebKitWebViewPrivate WebKitWebViewPrivate;

typedef enum {
    WEBKIT_NAVIGATION_RESPONSE_ACCEPT,
    WEBKIT_NAVIGATION_RESPONSE_IGNORE,
    WEBKIT_NAVIGATION_RESPONSE_DOWNLOAD
} WebKitNavigationResponse;

typedef enum
{
    WEBKIT_WEB_VIEW_TARGET_INFO_HTML,
    WEBKIT_WEB_VIEW_TARGET_INFO_TEXT,
    WEBKIT_WEB_VIEW_TARGET_INFO_IMAGE,
    WEBKIT_WEB_VIEW_TARGET_INFO_URI_LIST,
    WEBKIT_WEB_VIEW_TARGET_INFO_NETSCAPE_URL
} WebKitWebViewTargetInfo;

typedef enum
{
    WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED,
    WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING,
    WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN,
    WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED,
    WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED
} WebKitWebViewViewMode;

typedef enum
{
    WEBKIT_SELECTION_AFFINITY_UPSTREAM,
    WEBKIT_SELECTION_AFFINITY_DOWNSTREAM,
} WebKitSelectionAffinity;

typedef enum
{
    WEBKIT_INSERT_ACTION_TYPED,
    WEBKIT_INSERT_ACTION_PASTED,
    WEBKIT_INSERT_ACTION_DROPPED,
} WebKitInsertAction;

struct _WebKitWebView {
    GtkContainer parent_instance;

    /*< private >*/
    WebKitWebViewPrivate *priv;
};

struct _WebKitWebViewClass {
    GtkContainerClass parent_class;

    /*< public >*/
    /*
     * default handler/virtual methods
     */
    WebKitWebView            * (* create_web_view)        (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *web_frame);

    gboolean                   (* web_view_ready)          (WebKitWebView* web_view);

    gboolean                   (* close_web_view)         (WebKitWebView* web_view);

    WebKitNavigationResponse   (* navigation_requested)   (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           WebKitNetworkRequest *request);
    void                       (* window_object_cleared)  (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           JSGlobalContextRef    context,
                                                           JSObjectRef           window_object);
    gchar                    * (* choose_file)            (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           const gchar          *old_file);
    gboolean                   (* script_alert)           (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           const gchar          *alert_message);
    gboolean                   (* script_confirm)         (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           const gchar          *confirm_message,
                                                           gboolean             *did_confirm);
    gboolean                   (* script_prompt)          (WebKitWebView        *web_view,
                                                           WebKitWebFrame       *frame,
                                                           const gchar          *message,
                                                           const gchar          *default_value,
                                                           gchar*               *value);
    gboolean                   (* console_message)        (WebKitWebView        *web_view,
                                                           const gchar          *message,
                                                           guint                 line_number,
                                                           const gchar*          source_id);
    void                       (* select_all)             (WebKitWebView        *web_view);
    void                       (* cut_clipboard)          (WebKitWebView        *web_view);
    void                       (* copy_clipboard)         (WebKitWebView        *web_view);
    void                       (* paste_clipboard)        (WebKitWebView        *web_view);
    gboolean                   (* move_cursor)            (WebKitWebView        *web_view,
                                                           GtkMovementStep       step,
                                                           gint                  count);

    /*
     * internal
     */
    void                       (* set_scroll_adjustments) (WebKitWebView        *web_view,
                                                           GtkAdjustment        *hadjustment,
                                                           GtkAdjustment        *vadjustment);

    void                       (* undo)                   (WebKitWebView        *web_view);
    void                       (* redo)                   (WebKitWebView        *web_view);
    gboolean                   (* should_allow_editing_action) (WebKitWebView   *web_view);

    /* Padding for future expansion */
    void (*_webkit_reserved0) (void);
    void (*_webkit_reserved1) (void);
    void (*_webkit_reserved2) (void);
};

WEBKIT_API GType
webkit_web_view_get_type (void);

WEBKIT_API GtkWidget *
webkit_web_view_new (void);

WEBKIT_API const gchar *
webkit_web_view_get_title                       (WebKitWebView        *webView);


WEBKIT_API const gchar *
webkit_web_view_get_uri                         (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_maintains_back_forward_list (WebKitWebView        *webView,
                                                 gboolean              flag);

WEBKIT_API WebKitWebBackForwardList *
webkit_web_view_get_back_forward_list           (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_go_to_back_forward_item         (WebKitWebView        *webView,
                                                 WebKitWebHistoryItem *item);

WEBKIT_API gboolean
webkit_web_view_can_go_back                     (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_go_back_or_forward          (WebKitWebView        *webView,
                                                 gint                  steps);
WEBKIT_API gboolean
webkit_web_view_can_go_forward                  (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_go_back                         (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_go_back_or_forward              (WebKitWebView        *webView,
                                                 gint                  steps);
WEBKIT_API void
webkit_web_view_go_forward                      (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_stop_loading                    (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_open                            (WebKitWebView        *webView,
                                                 const gchar          *uri);

WEBKIT_API void
webkit_web_view_reload                          (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_reload_bypass_cache             (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_load_uri                        (WebKitWebView        *webView,
                                                 const gchar          *uri);
WEBKIT_API void
webkit_web_view_load_string                     (WebKitWebView        *webView,
                                                 const gchar          *content,
                                                 const gchar          *mime_type,
                                                 const gchar          *encoding,
                                                 const gchar          *base_uri);

WEBKIT_API void
webkit_web_view_load_html_string                (WebKitWebView        *webView,
                                                 const gchar          *content,
                                                 const gchar          *base_uri);

WEBKIT_API void
webkit_web_view_load_request                    (WebKitWebView        *webView,
                                                 WebKitNetworkRequest *request);

WEBKIT_API gboolean
webkit_web_view_search_text                     (WebKitWebView        *webView,
                                                 const gchar          *text,
                                                 gboolean              case_sensitive,
                                                 gboolean              forward,
                                                 gboolean              wrap);

WEBKIT_API guint
webkit_web_view_mark_text_matches               (WebKitWebView        *webView,
                                                 const gchar          *string,
                                                 gboolean              case_sensitive,
                                                 guint                 limit);

WEBKIT_API void
webkit_web_view_set_highlight_text_matches      (WebKitWebView        *webView,
                                                 gboolean              highlight);

WEBKIT_API void
webkit_web_view_unmark_text_matches             (WebKitWebView        *webView);

WEBKIT_API WebKitWebFrame *
webkit_web_view_get_main_frame                  (WebKitWebView        *webView);

WEBKIT_API WebKitWebFrame *
webkit_web_view_get_focused_frame               (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_execute_script                  (WebKitWebView        *webView,
                                                 const gchar          *script);

WEBKIT_API gboolean
webkit_web_view_can_cut_clipboard               (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_copy_clipboard              (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_paste_clipboard             (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_cut_clipboard                   (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_copy_clipboard                  (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_paste_clipboard                 (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_delete_selection                (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_has_selection                   (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_select_all                      (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_get_editable                    (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_editable                    (WebKitWebView        *webView,
                                                 gboolean              flag);

WEBKIT_API GtkTargetList *
webkit_web_view_get_copy_target_list            (WebKitWebView        *webView);

WEBKIT_API GtkTargetList *
webkit_web_view_get_paste_target_list           (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_settings                    (WebKitWebView        *webView,
                                                 WebKitWebSettings    *settings);

WEBKIT_API WebKitWebSettings *
webkit_web_view_get_settings                    (WebKitWebView        *webView);

WEBKIT_API WebKitWebInspector *
webkit_web_view_get_inspector                   (WebKitWebView        *webView);

WEBKIT_API WebKitWebWindowFeatures*
webkit_web_view_get_window_features             (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_show_mime_type              (WebKitWebView        *webView,
                                                 const gchar          *mime_type);

WEBKIT_API gboolean
webkit_web_view_get_transparent                 (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_transparent                 (WebKitWebView        *webView,
                                                 gboolean              flag);

WEBKIT_API gfloat
webkit_web_view_get_zoom_level                  (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_zoom_level                  (WebKitWebView        *webView,
                                                 gfloat                zoom_level);

WEBKIT_API void
webkit_web_view_zoom_in                         (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_zoom_out                        (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_get_full_content_zoom           (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_full_content_zoom           (WebKitWebView        *webView,
                                                 gboolean              full_content_zoom);

WEBKIT_API const gchar*
webkit_web_view_get_encoding                    (WebKitWebView        * webView);

WEBKIT_API void
webkit_web_view_set_custom_encoding             (WebKitWebView        * webView,
                                                 const gchar          * encoding);

WEBKIT_API const char*
webkit_web_view_get_custom_encoding             (WebKitWebView        * webView);

WEBKIT_API void
webkit_web_view_set_view_mode                   (WebKitWebView        *webView,
                                                 WebKitWebViewViewMode mode);

WEBKIT_API WebKitWebViewViewMode
webkit_web_view_get_view_mode                   (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_move_cursor                     (WebKitWebView        * webView,
                                                 GtkMovementStep        step,
                                                 gint                   count);

WEBKIT_API WebKitLoadStatus
webkit_web_view_get_load_status                 (WebKitWebView        *webView);

WEBKIT_API gdouble
webkit_web_view_get_progress                    (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_undo                            (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_undo                        (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_redo                            (WebKitWebView        *webView);

WEBKIT_API gboolean
webkit_web_view_can_redo                        (WebKitWebView        *webView);

WEBKIT_API void
webkit_web_view_set_view_source_mode            (WebKitWebView        *webView,
                                                 gboolean             view_source_mode);

WEBKIT_API gboolean
webkit_web_view_get_view_source_mode            (WebKitWebView        *webView);

WEBKIT_API WebKitHitTestResult*
webkit_web_view_get_hit_test_result             (WebKitWebView        *webView,
                                                 GdkEventButton       *event);

WEBKIT_API const gchar *
webkit_web_view_get_icon_uri                    (WebKitWebView        *webView);

WEBKIT_API GdkPixbuf *
webkit_web_view_get_icon_pixbuf                 (WebKitWebView        *webView);

WEBKIT_API WebKitDOMDocument *
webkit_web_view_get_dom_document                (WebKitWebView        *webView);

WEBKIT_API WebKitViewportAttributes*
webkit_web_view_get_viewport_attributes         (WebKitWebView        *webView);

G_END_DECLS

#endif
