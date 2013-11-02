m4_define([webkit_major_version], [2])
m4_define([webkit_minor_version], [3])
m4_define([webkit_micro_version], [1])

# This is the version we'll be using as part of our User-Agent string,
# e.g., AppleWebKit/$(webkit_user_agent_version) ...
#
# Sourced from Source/WebCore/Configurations/Version.xcconfig
m4_define([webkit_user_agent_major_version], [538])
m4_define([webkit_user_agent_minor_version], [4])

# Libtool library version, not to confuse with API version.
# See http://www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html
m4_define([libwebkitgtk_version], [20:0:20])
m4_define([libjavascriptcoregtk_version], [16:0:16])
m4_define([libwebkit2gtk_version], [30:0:5])

m4_define([gtk2_required_version], [2.24.10])
m4_define([gtk3_required_version], [3.6.0])
m4_define([gtk3_wayland_required_version], [3.10.0])

m4_define([atspi2_required_version], [2.5.3])
m4_define([cairo_required_version], [1.10])
m4_define([enchant_required_version], [0.22])
m4_define([fontconfig_required_version], [2.5])
m4_define([freetype2_required_version], [9.0])
m4_define([glib_required_version], [2.36.0])
m4_define([gobject_introspection_required_version], [1.32.0])
m4_define([gstreamer_plugins_base_required_version], [1.0.3])
m4_define([gstreamer_required_version], [1.0.3])
m4_define([harfbuzz_required_version], [0.9.7])
m4_define([libsoup_required_version], [2.42.0])
m4_define([libxml_required_version], [2.6])
m4_define([libxslt_required_version], [1.1.7])
m4_define([pango_required_version], [1.30.0])
m4_define([sqlite_required_version], [3.0])
