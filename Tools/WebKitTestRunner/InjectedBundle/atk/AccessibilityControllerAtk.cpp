/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityController.h"

#include "InjectedBundle.h"
#include <atk/atk.h>
#include <cstdio>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WTR {

static void printAccessibilityEvent(AtkObject* accessible, const gchar* signalName, const gchar* signalValue)
{
    // Do not handle state-change:defunct signals, as the AtkObject
    // associated to them will not be valid at this point already.
    if (!signalName || !g_strcmp0(signalName, "state-change:defunct"))
        return;

    if (!accessible || !ATK_IS_OBJECT(accessible))
        return;

    const gchar* objectName = atk_object_get_name(accessible);
    AtkRole objectRole = atk_object_get_role(accessible);

    // Try to always provide a name to be logged for the object.
    if (!objectName || *objectName == '\0')
        objectName = "(No name)";

    GOwnPtr<gchar> signalNameAndValue(signalValue ? g_strdup_printf("%s = %s", signalName, signalValue) : g_strdup(signalName));
    GOwnPtr<gchar> accessibilityEventString(g_strdup_printf("Accessibility object emitted \"%s\" / Name: \"%s\" / Role: %d\n", signalNameAndValue.get(), objectName, objectRole));
    InjectedBundle::shared().stringBuilder()->append(accessibilityEventString.get());
}

static gboolean axObjectEventListener(GSignalInvocationHint *signalHint, guint numParamValues, const GValue *paramValues, gpointer data)
{
    // At least we should receive the instance emitting the signal.
    if (numParamValues < 1)
        return TRUE;

    AtkObject* accessible = ATK_OBJECT(g_value_get_object(&paramValues[0]));
    if (!accessible || !ATK_IS_OBJECT(accessible))
        return TRUE;

    GSignalQuery signalQuery;
    GOwnPtr<gchar> signalName;
    GOwnPtr<gchar> signalValue;

    g_signal_query(signalHint->signal_id, &signalQuery);

    if (!g_strcmp0(signalQuery.signal_name, "state-change")) {
        signalName.set(g_strdup_printf("state-change:%s", g_value_get_string(&paramValues[1])));
        signalValue.set(g_strdup_printf("%d", g_value_get_boolean(&paramValues[2])));
    } else if (!g_strcmp0(signalQuery.signal_name, "focus-event")) {
        signalName.set(g_strdup("focus-event"));
        signalValue.set(g_strdup_printf("%d", g_value_get_boolean(&paramValues[1])));
    } else if (!g_strcmp0(signalQuery.signal_name, "children-changed")) {
        signalName.set(g_strdup("children-changed"));
        signalValue.set(g_strdup_printf("%d", g_value_get_uint(&paramValues[1])));
    } else if (!g_strcmp0(signalQuery.signal_name, "property-change"))
        signalName.set(g_strdup_printf("property-change:%s", g_quark_to_string(signalHint->detail)));
    else
        signalName.set(g_strdup(signalQuery.signal_name));

    printAccessibilityEvent(accessible, signalName.get(), signalValue.get());

    return TRUE;
}

void AccessibilityController::logAccessibilityEvents()
{
    // Ensure no callbacks are connected before.
    resetToConsistentState();

    // Ensure that accessibility is initialized for the WebView by querying for
    // the root accessible object, which will create the full hierarchy.
    rootElement();

    // Add global listeners for AtkObject's signals.
    m_stateChangeListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:state-change");
    m_focusEventListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:focus-event");
    m_activeDescendantChangedListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:active-descendant-changed");
    m_childrenChangedListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:children-changed");
    m_propertyChangedListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:property-change");
    m_visibleDataChangedListenerId = atk_add_global_event_listener(axObjectEventListener, "ATK:AtkObject:visible-data-changed");

    // Ensure the Atk interface types are registered, otherwise
    // the AtkDocument signal handlers below won't get registered.
    GObject* dummyAxObject = G_OBJECT(g_object_new(ATK_TYPE_OBJECT, 0));
    AtkObject* dummyNoOpAxObject = atk_no_op_object_new(dummyAxObject);
    g_object_unref(G_OBJECT(dummyNoOpAxObject));
    g_object_unref(dummyAxObject);
}

void AccessibilityController::resetToConsistentState()
{
    // AtkObject signals.
    if (m_stateChangeListenerId) {
        atk_remove_global_event_listener(m_stateChangeListenerId);
        m_stateChangeListenerId = 0;
    }
    if (m_focusEventListenerId) {
        atk_remove_global_event_listener(m_focusEventListenerId);
        m_focusEventListenerId = 0;
    }
    if (m_activeDescendantChangedListenerId) {
        atk_remove_global_event_listener(m_activeDescendantChangedListenerId);
        m_activeDescendantChangedListenerId = 0;
    }
    if (m_childrenChangedListenerId) {
        atk_remove_global_event_listener(m_childrenChangedListenerId);
        m_childrenChangedListenerId = 0;
    }
    if (m_propertyChangedListenerId) {
        atk_remove_global_event_listener(m_propertyChangedListenerId);
        m_propertyChangedListenerId = 0;
    }
    if (m_visibleDataChangedListenerId) {
        atk_remove_global_event_listener(m_visibleDataChangedListenerId);
        m_visibleDataChangedListenerId = 0;
    }
}

} // namespace WTR
