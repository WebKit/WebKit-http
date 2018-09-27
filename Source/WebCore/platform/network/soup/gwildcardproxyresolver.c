/*
* Copyright (c) 2016, Comcast
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>

#include <glib.h>

#include "gwildcardproxyresolver.h"

struct _GWildcardProxyResolverPrivate
{
    gchar *default_proxy;
    GPtrArray* proxies;
    GPtrArray* regex_wildcard;
};

static void free_proxies(gpointer data)
{
    GWildcardProxyResolverProxy *p = (GWildcardProxyResolverProxy*)(data);
    g_free(p->pattern);
    g_free(p->proxy);
    g_free(p);
}

static void free_regex_wildcard(gpointer data)
{
    GRegex *regex_wildcard = (GRegex*)data;
    g_regex_unref(regex_wildcard);
}

static void g_wildcard_proxy_resolver_iface_init(GProxyResolverInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GWildcardProxyResolver, g_wildcard_proxy_resolver, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(GWildcardProxyResolver)
                        G_IMPLEMENT_INTERFACE(G_TYPE_PROXY_RESOLVER,
                                               g_wildcard_proxy_resolver_iface_init))

enum
{
    PROP_DEFAULT_PROXY = 1
};

static void g_wildcard_proxy_resolver_set_property(GObject *object,
                                                   guint prop_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec)
{
    GWildcardProxyResolver *resolver = G_WILDCARD_PROXY_RESOLVER(object);

    switch (prop_id)
    {
    case PROP_DEFAULT_PROXY:
        g_wildcard_proxy_resolver_set_default_proxy(
                    resolver, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void g_wildcard_proxy_resolver_get_property(GObject *object,
                                                   guint prop_id,
                                                   GValue *value,
                                                   GParamSpec *pspec)
{
    GWildcardProxyResolver *resolver = G_WILDCARD_PROXY_RESOLVER(object);

    switch (prop_id)
    {
    case PROP_DEFAULT_PROXY:
        g_value_set_string(value, resolver->priv->default_proxy);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void g_wildcard_proxy_resolver_finalize(GObject *object)
{
    GWildcardProxyResolver *resolver = G_WILDCARD_PROXY_RESOLVER(object);
    GWildcardProxyResolverPrivate *priv = resolver->priv;

    g_free(priv->default_proxy);
    g_ptr_array_unref(priv->proxies);
    g_ptr_array_unref(priv->regex_wildcard);

    G_OBJECT_CLASS(g_wildcard_proxy_resolver_parent_class)->finalize(object);
}

static void g_wildcard_proxy_resolver_class_init(GWildcardProxyResolverClass *resolver_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(resolver_class);

    object_class->get_property = g_wildcard_proxy_resolver_get_property;
    object_class->set_property = g_wildcard_proxy_resolver_set_property;
    object_class->finalize = g_wildcard_proxy_resolver_finalize;

    g_object_class_install_property(object_class, PROP_DEFAULT_PROXY,
            g_param_spec_string("default-proxy",
                                "Default proxy",
                                "The default proxy URI",
                                 NULL,
                                 G_PARAM_READWRITE |
                                 G_PARAM_STATIC_STRINGS));
}

static void g_wildcard_proxy_resolver_init(GWildcardProxyResolver *resolver)
{
    resolver->priv = g_wildcard_proxy_resolver_get_instance_private(resolver);
}

static gchar* glob_to_regex(const gchar *glob_pattern)
{
    gint length = strlen(glob_pattern);
    GString *emulated = g_string_sized_new(length + 1);
    g_string_append_c(emulated, '^');
    for (gint i = 0; i < length; ++i)
    {
        switch (glob_pattern[i])
        {
        case '\0':
        case '\\':
        case '|':
        case '(':
        case ')':
        case '{':
        case '}':
        case '^':
        case '$':
        case '+':
        case '.':
            g_string_append_c(emulated, '\\');
            g_string_append_c(emulated, glob_pattern[i]);
            break;
        case '*':
            g_string_append_len(emulated, ".*", 2);
            break;
        case '?':
            g_string_append_c(emulated, '.');
            break;
        case '[':
            if (i + 1 < length && (glob_pattern[i + 1] == '^' ||
                                   glob_pattern[i + 1] == '!'))
            {
                g_string_append_len(emulated, "[^", 2);
                ++i;
            }
            else
            {
                g_string_append_c(emulated, '[');
            }
            break;
        default:
            g_string_append_c(emulated, glob_pattern[i]);
            break;
        }
    }
    g_string_append_c(emulated, '$');

    return g_string_free(emulated, FALSE);
}

static gchar** g_wildcard_proxy_resolver_lookup(GProxyResolver *proxy_resolver,
                                                const gchar *uri,
                                                GCancellable *cancellable,
                                                GError **error)
{
    GWildcardProxyResolver *resolver = G_WILDCARD_PROXY_RESOLVER(proxy_resolver);
    GWildcardProxyResolverPrivate *priv = resolver->priv;
    const gchar *proxy = NULL;
    gchar **proxies;

    for (guint i = 0; i < resolver->priv->regex_wildcard->len; ++i)
    {
        GRegex *regex_wildcard = (GRegex*)g_ptr_array_index(resolver->priv->regex_wildcard, i);
        if (g_regex_match(regex_wildcard, uri, (GRegexMatchFlags)0, NULL))
        {
            GWildcardProxyResolverProxy *p = (GWildcardProxyResolverProxy*)g_ptr_array_index(resolver->priv->proxies, i);
            if (!p->proxy || !strlen(p->proxy) || !strncmp(p->proxy, "direct", 6))
                proxy = "direct://";
            else
                proxy = p->proxy;
            break;
        }
    }

    if (!proxy)
        proxy = priv->default_proxy;
    if (!proxy)
        proxy = "direct://";

    if (!strncmp (proxy, "socks://", 8))
    {
        proxies = g_new0(gchar *, 4);
        proxies[0] = g_strdup_printf("socks5://%s", proxy + 8);
        proxies[1] = g_strdup_printf("socks4a://%s", proxy + 8);
        proxies[2] = g_strdup_printf("socks4://%s", proxy + 8);
        proxies[3] = NULL;
    }
    else
    {
        proxies = g_new0(gchar*, 2);
        proxies[0] = g_strdup(proxy);
    }

    return proxies;
}

static void g_wildcard_proxy_resolver_lookup_async(GProxyResolver *proxy_resolver,
                                                   const gchar *uri,
                                                   GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer user_data)
{
    GWildcardProxyResolver *resolver = G_WILDCARD_PROXY_RESOLVER(proxy_resolver);
    GTask *task;
    GError *error = NULL;
    char **proxies;

    task = g_task_new(resolver, cancellable, callback, user_data);

    proxies = g_wildcard_proxy_resolver_lookup(proxy_resolver, uri, cancellable, &error);
    if (proxies)
        g_task_return_pointer(task, proxies, (GDestroyNotify)g_strfreev);
    else
        g_task_return_error(task, error);
    g_object_unref(task);
}

static gchar** g_wildcard_proxy_resolver_lookup_finish(GProxyResolver *resolver,
                                                       GAsyncResult *result,
                                                       GError **error)
{
    g_return_val_if_fail(g_task_is_valid(result, resolver), NULL);

    return g_task_propagate_pointer(G_TASK(result), error);
}

static void g_wildcard_proxy_resolver_iface_init(GProxyResolverInterface *iface)
{
    iface->lookup = g_wildcard_proxy_resolver_lookup;
    iface->lookup_async = g_wildcard_proxy_resolver_lookup_async;
    iface->lookup_finish = g_wildcard_proxy_resolver_lookup_finish;
}

/**
 * g_wildcard_proxy_resolver_new:
 * @default_proxy: (allow-none): the default proxy to use
 *
 * Creates a new #GWildcardProxyResolver
 *
 * Returns: (transfer full) a new #GWildcardProxyResolver
 */
GProxyResolver* g_wildcard_proxy_resolver_new(const gchar *default_proxy)
{
    return g_object_new(G_TYPE_WILDCARD_PROXY_RESOLVER,
                        "default-proxy", default_proxy, NULL);
}

/**
 * g_wildcard_proxy_resolver_set_default_proxy:
 * @resolver: a #GWildcardProxyResolver
 * @default_proxy: the default proxy to use
 *
 * Sets the default proxy on @resolver, to be used for any URIs that
 * don't match provided proxy patterns
 */
void g_wildcard_proxy_resolver_set_default_proxy(GWildcardProxyResolver *resolver,
                                                 const gchar *default_proxy)
{
    g_return_if_fail(G_IS_WILDCARD_PROXY_RESOLVER(resolver));

    g_free(resolver->priv->default_proxy);
    resolver->priv->default_proxy = g_strdup(default_proxy);
    g_object_notify(G_OBJECT(resolver), "default-proxy");
}

/**
 * g_wildcard_proxy_resolver_set_proxies:
 * @resolver: a #GWildcardProxyResolver
 * @proxies: (transfer full): array of proxies
 *
 * Patterns of the proxies are checked in a specified order
 * to choose which proxy to use
 */
void g_wildcard_proxy_resolver_set_proxies(GWildcardProxyResolver  *resolver,
                                           GPtrArray *proxies)
{
    g_return_if_fail(G_IS_WILDCARD_PROXY_RESOLVER(resolver));

    resolver->priv->proxies = proxies;

    const guint length = proxies->len;
    resolver->priv->regex_wildcard = g_ptr_array_sized_new(length);

    for (guint i = 0; i < length; ++i)
    {
        GError *error = NULL;
        GWildcardProxyResolverProxy *p = (GWildcardProxyResolverProxy*)g_ptr_array_index(proxies, i);
        gchar *regex_wildcard = glob_to_regex(p->pattern);
        g_ptr_array_add(resolver->priv->regex_wildcard,
                        g_regex_new(regex_wildcard,
                                    G_REGEX_UNGREEDY,
                                    (GRegexMatchFlags)0,
                                    &error));
        g_free(regex_wildcard);
    }
    g_ptr_array_set_free_func(resolver->priv->proxies, free_proxies);
    g_ptr_array_set_free_func(resolver->priv->regex_wildcard,
                              free_regex_wildcard);
}
