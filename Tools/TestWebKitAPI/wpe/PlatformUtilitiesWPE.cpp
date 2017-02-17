
#include "config.h"
#include "PlatformUtilities.h"

#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GUniquePtr.h>

namespace TestWebKitAPI {
namespace Util {

void sleep(double seconds)
{
    g_usleep(seconds * 1000000);
}

static char* getFilenameFromEnvironmentVariableAsUTF8(const char* variableName)
{
    const char* value = g_getenv(variableName);
    if (!value) {
        g_printerr("%s environment variable not found\n", variableName);
        exit(1);
    }
    gsize bytesWritten;
    return g_filename_to_utf8(value, -1, 0, &bytesWritten, 0);
}

WKStringRef createInjectedBundlePath()
{
    GUniquePtr<char> injectedBundlePath(getFilenameFromEnvironmentVariableAsUTF8("TEST_WEBKIT_API_WEBKIT2_INJECTED_BUNDLE_PATH"));
    GUniquePtr<char> injectedBundleFilename(g_build_filename(injectedBundlePath.get(), "libTestWebKitAPIInjectedBundle.so", nullptr));
    return WKStringCreateWithUTF8CString(injectedBundleFilename.get());
}

WKURLRef createURLForResource(const char* resource, const char* extension)
{
    GUniquePtr<char> testResourcesPath(getFilenameFromEnvironmentVariableAsUTF8("TEST_WEBKIT_API_WEBKIT2_RESOURCES_PATH"));
    GUniquePtr<char> resourceBasename(g_strdup_printf("%s.%s", resource, extension));
    GUniquePtr<char> resourceFilename(g_build_filename(testResourcesPath.get(), resourceBasename.get(), nullptr));
    GRefPtr<GFile> resourceFile = adoptGRef(g_file_new_for_path(resourceFilename.get()));
    GUniquePtr<char> resourceURI(g_file_get_uri(resourceFile.get()));
    return WKURLCreateWithUTF8CString(resourceURI.get());
}

} // namespace Util
} // namespace TestWebKitAPI
