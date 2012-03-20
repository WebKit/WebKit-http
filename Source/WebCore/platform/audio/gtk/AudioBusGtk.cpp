/*
 *  Copyright (C) 2011 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioBus.h"

#include "AudioFileReader.h"
#include "FileSystem.h"

#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#include <gio/gio.h>
#include <glib.h>

namespace WebCore {

PassOwnPtr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{
    GOwnPtr<gchar> filename(g_strdup_printf("%s.wav", name));
    GOwnPtr<gchar> absoluteFilename(g_build_filename(sharedResourcesPath().data(), "resources", "audio", filename.get(), NULL));

    GFile* file = g_file_new_for_path(filename.get());
    if (!g_file_query_exists(file, 0)) {
        // Uninstalled case, assume we're in the WebKit root directory.
        const char* environmentPath = getenv("AUDIO_RESOURCES_PATH");
        absoluteFilename.set(g_build_filename(environmentPath, filename.get(), NULL));
    }
    return createBusFromAudioFile(absoluteFilename.get(), false, sampleRate);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
