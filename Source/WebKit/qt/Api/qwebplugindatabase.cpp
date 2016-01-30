/*
    Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>

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

#include "config.h"
#include "qwebplugindatabase_p.h"

#if !PLUGIN_VIEW_IS_BROKEN
#include "PluginDatabase.h"
#include "PluginPackage.h"
#endif

using namespace WebCore;

/*!
    \internal
    \typedef QWebPluginInfo::MimeType
    \since 4.6
    \brief Represents a single MIME type supported by a plugin.
*/

/*!
    \class QWebPluginInfo
    \internal
    \since 4.6
    \brief The QWebPluginInfo class represents a single Netscape plugin.

    A QWebPluginInfo object represents a Netscape plugin picked up by WebKit
    and included in the plugin database. This class contains information about
    the plugin, such as its name(), description(), a list of MIME types that it
    supports (can be accessed with mimeTypes()) and the path of the plugin
    file.

    Plugins can be enabled and disabled with setEnabled(). If a plugin is
    disabled, it will not be used by WebKit to handle supported MIME types. To
    check if a plugin is enabled or not, use enabled().

    \sa QWebPluginDatabase
*/

/*!
    Constructs a null QWebPluginInfo.
*/
QWebPluginInfo::QWebPluginInfo()
#if !PLUGIN_VIEW_IS_BROKEN
    : m_package(0)
#endif
{
}

#if !PLUGIN_VIEW_IS_BROKEN
QWebPluginInfo::QWebPluginInfo(PluginPackage* package)
    : m_package(package)
{
    if (m_package)
        m_package->ref();
}
#endif

/*!
    Contructs a copy of \a other.
*/
QWebPluginInfo::QWebPluginInfo(const QWebPluginInfo& other)
#if !PLUGIN_VIEW_IS_BROKEN
    : m_package(other.m_package)
#endif
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (m_package)
        m_package->ref();
#endif
}

/*!
    Destroys the plugin info.
*/
QWebPluginInfo::~QWebPluginInfo()
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (m_package)
        m_package->deref();
#endif
}

/*!
    Returns the name of the plugin.

    \sa description()
*/
QString QWebPluginInfo::name() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return QString();
    return m_package->name();
#else
    return QString();
#endif
}

/*!
    Returns the description of the plugin.

    \sa name()
*/
QString QWebPluginInfo::description() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return QString();
    return m_package->description();
#else
    return QString();
#endif
}

/*!
    Returns a list of MIME types supported by the plugin.

    \sa supportsMimeType()
*/
QList<QWebPluginInfo::MimeType> QWebPluginInfo::mimeTypes() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (m_package && m_mimeTypes.isEmpty()) {
        const MIMEToDescriptionsMap& mimeToDescriptions = m_package->mimeToDescriptions();
        MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();

        for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it) {
            MimeType mimeType;
            mimeType.name = it->key;
            mimeType.description = it->value;

            QStringList fileExtensions;
            Vector<String> extensions = m_package->mimeToExtensions().get(mimeType.name);

            for (unsigned i = 0; i < extensions.size(); ++i)
                fileExtensions.append(extensions[i]);

            mimeType.fileExtensions = fileExtensions;
            m_mimeTypes.append(mimeType);
        }
    }
#endif

    return m_mimeTypes;
}

/*!
    Returns true if the plugin supports a specific \a mimeType; otherwise
    returns false.

    \sa mimeTypes()
*/
bool QWebPluginInfo::supportsMimeType(const QString& mimeType) const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return false;
    return m_package->mimeToDescriptions().contains(mimeType);
#else
    return false;
#endif
}

/*!
    Returns an absolute path to the plugin file.
*/
QString QWebPluginInfo::path() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return QString();
    return m_package->path();
#else
    return QString();
#endif
}

/*!
    Returns true if the plugin is a null plugin; otherwise returns false.
*/
bool QWebPluginInfo::isNull() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    return !m_package;
#else
    return true;
#endif
}

/*!
    Enables or disables the plugin, depending on the \a enabled parameter.

    Disabled plugins will not be picked up by WebKit when looking for a plugin
    supporting a particular MIME type.

    \sa isEnabled()
*/
void QWebPluginInfo::setEnabled(bool enabled)
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return;
    m_package->setEnabled(enabled);
#endif
}

/*!
    Returns true if the plugin is enabled; otherwise returns false.

    \sa setEnabled()
*/
bool QWebPluginInfo::isEnabled() const
{
#if !PLUGIN_VIEW_IS_BROKEN
    if (!m_package)
        return false;
    return m_package->isEnabled();
#else
    return false;
#endif
}

/*!
    Returns true if this plugin info is the same as the \a other plugin info.
*/
bool QWebPluginInfo::operator==(const QWebPluginInfo& other) const
{
#if !PLUGIN_VIEW_IS_BROKEN
    return m_package == other.m_package;
#else
    return true;
#endif
}

/*!
    Returns true if this plugin info is different from the \a other plugin info.
*/
bool QWebPluginInfo::operator!=(const QWebPluginInfo& other) const
{
#if !PLUGIN_VIEW_IS_BROKEN
    return m_package != other.m_package;
#else
    return false;
#endif
}

/*!
    Assigns the \a other plugin info to this plugin info, and returns a reference
    to this plugin info.
*/
QWebPluginInfo &QWebPluginInfo::operator=(const QWebPluginInfo& other)
{
    if (this == &other)
        return *this;

#if !PLUGIN_VIEW_IS_BROKEN
    if (m_package)
        m_package->deref();
    m_package = other.m_package;
    if (m_package)
        m_package->ref();
#endif
    m_mimeTypes = other.m_mimeTypes;

    return *this;
}

/*!
    \class QWebPluginDatabase
    \internal
    \since 4.6
    \brief The QWebPluginDatabase class provides an interface for managing
    Netscape plugins used by WebKit in QWebPages.

    The QWebPluginDatabase class is a database of Netscape plugins that are used
    by WebKit. The plugins are picked up by WebKit by looking up a set of search paths.
    The default set can be accessed using defaultSearchPaths(). The search paths
    can be changed, see searchPaths() and setSearchPaths(). Additional search paths
    can also be added using addSearchPath().

    The plugins that have been detected are exposed by the plugins() method.
    The list contains QWebPlugin objects that hold both the metadata and the MIME
    types that are supported by particular plugins.

    WebKit specifies a plugin for a MIME type by looking for the first plugin that
    supports the specific MIME type. To get a plugin, that is used by WebKit to
    handle a specific MIME type, you can use the pluginForMimeType() function.

    To change the way of resolving MIME types ambiguity, you can explicitly set
    a preferred plugin for a specific MIME type, using setPreferredPluginForMimeType().

    \sa QWebPluginInfo, QWebSettings::pluginDatabase()
*/

QWebPluginDatabase::QWebPluginDatabase(QObject* parent)
    : QObject(parent)
#if !PLUGIN_VIEW_IS_BROKEN
    , m_database(PluginDatabase::installedPlugins())
#endif
{
}

QWebPluginDatabase::~QWebPluginDatabase()
{
}

/*!
    Returns a list of plugins installed in the search paths.

    This list will contain disabled plugins, although they will not be used by
    WebKit.

    \sa pluginForMimeType()
*/
QList<QWebPluginInfo> QWebPluginDatabase::plugins() const
{
    QList<QWebPluginInfo> qwebplugins;
#if !PLUGIN_VIEW_IS_BROKEN
    const Vector<PluginPackage*>& plugins = m_database->plugins();

    for (unsigned int i = 0; i < plugins.size(); ++i) {
        PluginPackage* plugin = plugins[i];
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE) 
        if (!plugin->ensurePluginLoaded())
            continue;
#endif
        qwebplugins.append(QWebPluginInfo(plugin));
    }
#endif

    return qwebplugins;
}

/*!
    Returns a default set of search paths.

    \sa searchPaths(), setSearchPaths()
*/
QStringList QWebPluginDatabase::defaultSearchPaths()
{
    QStringList paths;

#if !PLUGIN_VIEW_IS_BROKEN
    const Vector<String>& directories = PluginDatabase::defaultPluginDirectories();
    for (unsigned int i = 0; i < directories.size(); ++i)
        paths.append(directories[i]);
#endif

    return paths;
}

/*!
    Returns a list of search paths that are used by WebKit to look for plugins.

    \sa defaultSearchPaths(), setSearchPaths()
*/
QStringList QWebPluginDatabase::searchPaths() const
{
    QStringList paths;

#if !PLUGIN_VIEW_IS_BROKEN
    const Vector<String>& directories = m_database->pluginDirectories();
    for (unsigned int i = 0; i < directories.size(); ++i)
        paths.append(directories[i]);
#endif

    return paths;
}

/*!
    Changes the search paths to \a paths.
    The database is automatically refreshed.

    \sa searchPaths(), defaultSearchPaths()
*/
void QWebPluginDatabase::setSearchPaths(const QStringList& paths)
{
#if !PLUGIN_VIEW_IS_BROKEN
    Vector<String> directories;

    for (int i = 0; i < paths.count(); ++i)
        directories.append(paths.at(i));

    m_database->setPluginDirectories(directories);
    // PluginDatabase::setPluginDirectories() does not refresh the database.
    m_database->refresh();
#endif
}

/*!
    Adds an additional \a path to the current set.
    The database is automatically refreshed.

    \sa searchPaths(), setSearchPaths()
*/
void QWebPluginDatabase::addSearchPath(const QString& path)
{
#if !PLUGIN_VIEW_IS_BROKEN
    m_database->addExtraPluginDirectory(path);
    // PluginDatabase::addExtraPluginDirectory() does refresh the database.
#endif
}

/*!
    Refreshes the plugin database, adds new plugins that have been found and removes
    the ones that are no longer available in the search paths.

    You can call this function when the set of plugins installed in the search paths
    changes. You do not need to call this function when changing search paths,
    in that case WebKit automatically refreshes the database.
*/
void QWebPluginDatabase::refresh()
{
#if !PLUGIN_VIEW_IS_BROKEN
    m_database->refresh();
#endif
}

/*!
    Returns the plugin that is currently used by WebKit for a given \a mimeType.

    \sa setPreferredPluginForMimeType()
*/
QWebPluginInfo QWebPluginDatabase::pluginForMimeType(const QString& mimeType)
{
#if !PLUGIN_VIEW_IS_BROKEN
    return QWebPluginInfo(m_database->pluginForMIMEType(mimeType));
#else
    return QWebPluginInfo();
#endif
}

/*!
    Changes the preferred plugin for a given \a mimeType to \a plugin. The \a plugin
    has to support the given \a mimeType, otherwise the setting will have no effect.

    Calling the function with a null \a plugin resets the setting.

    \sa pluginForMimeType()
*/
void QWebPluginDatabase::setPreferredPluginForMimeType(const QString& mimeType, const QWebPluginInfo& plugin)
{
#if !PLUGIN_VIEW_IS_BROKEN
    m_database->setPreferredPluginForMIMEType(mimeType, plugin.m_package);
#endif
}
