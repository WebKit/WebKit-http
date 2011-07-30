/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/**
 * @file    ewk_frame.h
 * @brief   WebKit frame smart object.
 *
 * This object is the low level access to WebKit-EFL browser
 * component. It represents both the main and internal frames that
 * HTML pages contain.
 *
 * Every ewk_view has at least one frame, called "main frame" and
 * retrieved with ewk_view_frame_main_get(). One can retrieve frame's
 * owner view with ewk_frame_view_get(). Parent frame can be retrieved
 * with standard smart object's evas_object_smart_parent_get().
 * Children can be accessed with ewk_frame_children_iterator_new() or
 * ewk_frame_child_find().
 *
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 *
 *  - "title,changed", const char*: title of the main frame was changed.
 *  - "uri,changed", const char*: uri of the main frame was changed.
 *  - "load,document,finished", void: frame finished loading the document.
 *  - "load,nonemptylayout,finished", void: frame finished first
 *    non-empty layout.
 *  - "load,started", void: frame started loading the document.
 *  - "load,progress", double*: load progress is changed (overall value
 *    from 0.0 to 1.0, connect to individual frames for fine grained).
 *  - "load,finished", const Ewk_Frame_Load_Error*: reports load
 *    finished and it gives @c NULL on success or pointer to
 *    structure defining the error.
 *  - "load,provisional", void: frame started provisional load.
 *  - "load,firstlayout,finished", void: frame finished first layout.
 *  - "load,error", const Ewk_Frame_Load_Error*: reports load failed
 *    and it gives a pointer to structure defining the error as an argument.
 *  - "contents,size,changed", Evas_Coord[2]: reports contents size
 *     were changed due new layout, script actions or any other events.
 *  - "navigation,first", void: first navigation was occurred.
 *  - "resource,request,new", Ewk_Frame_Resource_Request*: reports that
 *    there's a new resource request.
 *  - "resource,request,willsend", Ewk_Frame_Resource_Request*: a resource will
 *    be requested.
 *  - "state,save", void: frame's state will be saved as a history item.
 *  - "editorclient,contents,changed", void: reports that editor client's
 *    contents were changed
 */

#ifndef ewk_frame_h
#define ewk_frame_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Creates a type name for _Ewk_Frame_Load_Error.
typedef struct _Ewk_Frame_Load_Error Ewk_Frame_Load_Error;
/**
 * @brief   Structure used to report load errors.
 *
 * Load errors are reported as signal by ewk_view. All the strings are
 * temporary references and should @b not be used after the signal
 * callback returns. If it's required, make copies with strdup() or
 * eina_stringshare_add() (they are not even guaranteed to be
 * stringshared, so must use eina_stringshare_add() and not
 * eina_stringshare_ref()).
 */
struct _Ewk_Frame_Load_Error {
    int code; /**< numeric error code */
    Eina_Bool is_cancellation; /**< if load failed because it was canceled */
    const char *domain; /**< error domain name */
    const char *description; /**< error description already localized */
    const char *failing_url; /**< the url that failed to load */
    Evas_Object *frame; /**< frame where the failure happened */
};

/// Creates a type name for _Ewk_Frame_Resource_Request.
typedef struct _Ewk_Frame_Resource_Request Ewk_Frame_Resource_Request;
/**
 * @brief   Structure used to report resource request.
 *
 * Details given before a resource is loaded on a given frame. It's used by
 * ewk_frame_request_will_send() to inform the details of a to-be-loaded
 * resource, allowing them to be overridden.
 */
struct _Ewk_Frame_Resource_Request {
    const char *url; /**< url of the resource */
    const unsigned long identifier; /**< identifier of resource, can not be changed */
};

/// Enum containing hit test data types
typedef enum {
    EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT = 1 << 1,
    EWK_HIT_TEST_RESULT_CONTEXT_LINK = 1 << 2,
    EWK_HIT_TEST_RESULT_CONTEXT_IMAGE = 1 << 3,
    EWK_HIT_TEST_RESULT_CONTEXT_MEDIA = 1 << 4,
    EWK_HIT_TEST_RESULT_CONTEXT_SELECTION = 1 << 5,
    EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE = 1 << 6
} Ewk_Hit_Test_Result_Context;

/// Creates a type name for _Ewk_Hit_Test.
typedef struct _Ewk_Hit_Test Ewk_Hit_Test;
/// Structure used to report hit test result.
struct _Ewk_Hit_Test {
    int x; /**< the horizontal position of the hit test */
    int y; /**< the vertical position of the hit test */
    struct {
        int x, y, w, h;
    } bounding_box; /**< DEPRECATED, see ewk_frame_hit_test_new() */
    const char *title; /**< title of the element */
    const char *alternate_text; /**< the alternate text for image, area, input and applet */
    Evas_Object *frame; /**< the pointer to frame where hit test was requested */
    struct {
        const char *text; /**< the text of the link */
        const char *url; /**< URL of the link */
        const char *title; /**< the title of link */
        Evas_Object *target_frame;
    } link;
    const char *image_uri;
    const char *media_uri;

    Ewk_Hit_Test_Result_Context context;
};

/// Represents actions of touch events.
typedef enum {
    EWK_TOUCH_START,
    EWK_TOUCH_END,
    EWK_TOUCH_MOVE,
    EWK_TOUCH_CANCEL
} Ewk_Touch_Event_Type;

/// Represents states of touch events.
typedef enum {
    EWK_TOUCH_POINT_PRESSED,
    EWK_TOUCH_POINT_RELEASED,
    EWK_TOUCH_POINT_MOVED,
    EWK_TOUCH_POINT_CANCELLED
} Ewk_Touch_Point_Type;

/// Creates a type name for _Ewk_Touch_Point.
typedef struct _Ewk_Touch_Point Ewk_Touch_Point;
/// Represents a touch point.
struct _Ewk_Touch_Point {
    unsigned int id; /**< identifier of the touch event */
    int x; /**< the horizontal position of the touch event */
    int y; /**< the horizontal position of the touch event */
    Ewk_Touch_Point_Type state; /**< state of the touch event */
};

typedef enum {
    EWK_TEXT_SELECTION_NONE,
    EWK_TEXT_SELECTION_CARET,
    EWK_TEXT_SELECTION_RANGE
} Ewk_Text_Selection_Type;

EAPI Evas_Object *ewk_frame_view_get(const Evas_Object *o);

EAPI Eina_Iterator *ewk_frame_children_iterator_new(Evas_Object *o);
EAPI Evas_Object   *ewk_frame_child_find(Evas_Object *o, const char *name);

EAPI Eina_Bool    ewk_frame_uri_set(Evas_Object *o, const char *uri);
EAPI const char  *ewk_frame_uri_get(const Evas_Object *o);
EAPI const char  *ewk_frame_title_get(const Evas_Object *o);
EAPI const char  *ewk_frame_name_get(const Evas_Object *o);
EAPI Eina_Bool    ewk_frame_contents_size_get(const Evas_Object *o, Evas_Coord *w, Evas_Coord *h);

EAPI Eina_Bool    ewk_frame_contents_set(Evas_Object *o, const char *contents, size_t contents_size, const char *mime_type, const char *encoding, const char *base_uri);
EAPI Eina_Bool    ewk_frame_contents_alternate_set(Evas_Object *o, const char *contents, size_t contents_size, const char *mime_type, const char *encoding, const char *base_uri, const char *unreachable_uri);

EAPI Eina_Bool    ewk_frame_script_execute(Evas_Object *o, const char *script);

EAPI Eina_Bool    ewk_frame_editable_get(const Evas_Object *o);
EAPI Eina_Bool    ewk_frame_editable_set(Evas_Object *o, Eina_Bool editable);

EAPI char        *ewk_frame_selection_get(const Evas_Object *o);

EAPI Eina_Bool    ewk_frame_text_search(const Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool forward, Eina_Bool wrap);

EAPI unsigned int ewk_frame_text_matches_mark(Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool highlight, unsigned int limit);
EAPI Eina_Bool    ewk_frame_text_matches_unmark_all(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_text_matches_highlight_set(Evas_Object *o, Eina_Bool highlight);
EAPI Eina_Bool    ewk_frame_text_matches_highlight_get(const Evas_Object *o);
EAPI Eina_Bool    ewk_frame_text_matches_nth_pos_get(Evas_Object *o, size_t n, int *x, int *y);

EAPI Eina_Bool    ewk_frame_stop(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_reload(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_reload_full(Evas_Object *o);

EAPI Eina_Bool    ewk_frame_back(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_forward(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_navigate(Evas_Object *o, int steps);

EAPI Eina_Bool    ewk_frame_back_possible(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_forward_possible(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_navigate_possible(Evas_Object *o, int steps);

EAPI float        ewk_frame_zoom_get(const Evas_Object *o);
EAPI Eina_Bool    ewk_frame_zoom_set(Evas_Object *o, float zoom);
EAPI Eina_Bool    ewk_frame_zoom_text_only_get(const Evas_Object *o);
EAPI Eina_Bool    ewk_frame_zoom_text_only_set(Evas_Object *o, Eina_Bool setting);

EAPI void          ewk_frame_hit_test_free(Ewk_Hit_Test *hit_test);
EAPI Ewk_Hit_Test *ewk_frame_hit_test_new(const Evas_Object *o, int x, int y);

EAPI Eina_Bool    ewk_frame_scroll_add(Evas_Object *o, int dx, int dy);
EAPI Eina_Bool    ewk_frame_scroll_set(Evas_Object *o, int x, int y);

EAPI Eina_Bool    ewk_frame_scroll_size_get(const Evas_Object *o, int *w, int *h);
EAPI Eina_Bool    ewk_frame_scroll_pos_get(const Evas_Object *o, int *x, int *y);

EAPI Eina_Bool    ewk_frame_visible_content_geometry_get(const Evas_Object *o, Eina_Bool include_scrollbars, int *x, int *y, int *w, int *h);

EAPI Eina_Bool    ewk_frame_paint_full_get(const Evas_Object *o);
EAPI void         ewk_frame_paint_full_set(Evas_Object *o, Eina_Bool flag);

EAPI Eina_Bool    ewk_frame_feed_focus_in(Evas_Object *o);
EAPI Eina_Bool    ewk_frame_feed_focus_out(Evas_Object *o);

EAPI Eina_Bool    ewk_frame_feed_mouse_wheel(Evas_Object *o, const Evas_Event_Mouse_Wheel *ev);
EAPI Eina_Bool    ewk_frame_feed_mouse_down(Evas_Object *o, const Evas_Event_Mouse_Down *ev);
EAPI Eina_Bool    ewk_frame_feed_mouse_up(Evas_Object *o, const Evas_Event_Mouse_Up *ev);
EAPI Eina_Bool    ewk_frame_feed_mouse_move(Evas_Object *o, const Evas_Event_Mouse_Move *ev);
EAPI Eina_Bool    ewk_frame_feed_touch_event(Evas_Object *o, Ewk_Touch_Event_Type action, Eina_List *points, int metaState);
EAPI Eina_Bool    ewk_frame_feed_key_down(Evas_Object *o, const Evas_Event_Key_Down *ev);
EAPI Eina_Bool    ewk_frame_feed_key_up(Evas_Object *o, const Evas_Event_Key_Up *ev);

EAPI Ewk_Text_Selection_Type ewk_frame_text_selection_type_get(Evas_Object *o);


#ifdef __cplusplus
}
#endif
#endif // ewk_frame_h
