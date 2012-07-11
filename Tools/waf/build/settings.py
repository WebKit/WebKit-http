# Copyright (C) 2009 Kevin Ollivier  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Common elements of the waf build system shared by all projects.

import commands
import os
import platform
import re
import sys

import Logs
import Options

from build_utils import *
from waf_extensions import *

# to be moved to wx when it supports more configs
from wxpresets import *

wk_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))

if sys.platform.startswith('win'):
    if not 'WXWIN' in os.environ:
        print "Please set WXWIN to the directory containing wxWidgets."
        sys.exit(1)

    wx_root = os.environ['WXWIN']
else:
    wx_root = commands.getoutput('wx-config --prefix')

wtf_dir = os.path.join(wk_root, 'Source', 'WTF')
jscore_dir = os.path.join(wk_root, 'Source', 'JavaScriptCore')
webcore_dir = os.path.join(wk_root, 'Source', 'WebCore')
wklibs_dir = os.path.join(wk_root, 'WebKitLibraries')

common_defines = []
common_cxxflags = []
common_includes = []
common_libs = []
common_libpaths = []
common_frameworks = []

ports = [
    'BlackBerry',
    'Chromium',
    'Efl',
    'Gtk',
    'Mac',
    'None',
    'Qt',
    'Safari',
    'Win',
    'WinCE',
    'wx',
]

uses = [
    'CF',
    'CFNet',
    'CURL',
    'WXGC',
]

port_uses = {
    'wx': ['CURL', 'WXGC'],
}

jscore_dirs = [
    'API',
    'assembler',
    'bytecode',
    'bytecompiler',
    'debugger',
    'disassembler',
    'disassembler/udis86',
    'DerivedSources',
    'dfg',
    'heap',
    'interpreter',
    'jit',
    'llint',
    'parser',
    'profiler',
    'runtime',
    'tools',
    'yarr',
    '../WTF/wtf',
    '../WTF/wtf/dtoa',
    '../WTF/wtf/text',
    '../WTF/wtf/unicode',
    '../WTF/wtf/unicode/icu',
]

webcore_dirs_common = [
    'Source/WebCore/Modules/websockets',
    'Source/WebCore/accessibility',
    'Source/WebCore/bindings',
    'Source/WebCore/bindings/cpp',
    'Source/WebCore/bindings/generic',
    'Source/WebCore/bindings/js',
    'Source/WebCore/bridge',
    'Source/WebCore/bridge/c',
    'Source/WebCore/bridge/jsc',
    'Source/WebCore/css',
    'Source/WebCore/DerivedSources',
    'Source/WebCore/dom',
    'Source/WebCore/dom/default',
    'Source/WebCore/editing',
    'Source/WebCore/fileapi',
    'Source/WebCore/history',
    'Source/WebCore/html',
    'Source/WebCore/html/canvas',
    'Source/WebCore/html/parser',
    'Source/WebCore/html/shadow',
    'Source/WebCore/inspector',
    'Source/WebCore/loader',
    'Source/WebCore/loader/appcache',
    'Source/WebCore/loader/archive',
    'Source/WebCore/loader/cache',
    'Source/WebCore/loader/icon',
    'Source/WebCore/Modules/filesystem',
    'Source/WebCore/Modules/geolocation',
    'Source/WebCore/Modules/notifications',
    'Source/WebCore/Modules/indexeddb',
    'Source/WebCore/Modules/quota',
    'Source/WebCore/Modules/webdatabase',
    'Source/WebCore/page',
    'Source/WebCore/page/animation',
    'Source/WebCore/page/scrolling',
    'Source/WebCore/platform',
    'Source/WebCore/platform/animation',
    'Source/WebCore/platform/graphics',
    'Source/WebCore/platform/graphics/filters',
    'Source/WebCore/platform/graphics/filters/arm',
    'Source/WebCore/platform/graphics/transforms',
    'Source/WebCore/platform/image-decoders',
    'Source/WebCore/platform/image-decoders/bmp',
    'Source/WebCore/platform/image-decoders/gif',
    'Source/WebCore/platform/image-decoders/ico',
    'Source/WebCore/platform/image-decoders/jpeg',
    'Source/WebCore/platform/image-decoders/png',
    'Source/WebCore/platform/image-decoders/webp',
    'Source/WebCore/platform/mock',
    'Source/WebCore/platform/network',
    'Source/WebCore/platform/posix',
    'Source/WebCore/platform/sql',
    'Source/WebCore/platform/text',
    'Source/WebCore/platform/text/transcoder',
    'Source/WebCore/plugins',
    'Source/WebCore/rendering',
    'Source/WebCore/rendering/style',
    'Source/WebCore/rendering/svg',
    'Source/WebCore/storage',
    'Source/WebCore/svg',
    'Source/WebCore/svg/animation',
    'Source/WebCore/svg/graphics',
    'Source/WebCore/svg/graphics/filters',
    'Source/WebCore/svg/properties',
    'Source/WebCore/testing',
    'Source/WebCore/testing/js',
    'Source/WebCore/workers',
    'Source/WebCore/xml',
    'Source/WebCore/xml/parser',
]

config = get_config(wk_root)
arch = get_arch(wk_root)
config_dir = config

output_dir = os.path.join(get_base_product_dir(wk_root), config_dir)

building_on_win32 = sys.platform.startswith('win')

default_port = 'Mac'
if building_on_win32:
    deafult_port = "Win"

build_port = default_port

def get_port_excludes(thisport):
    """
    This function creates a list of file patterns to exclude
    based on what port you are using and what USES it has defined.
    """
    exclude_patterns = []
    global ports
    for port in ports:
        if not port == thisport:
            exclude = "*%s.cpp" % port
            if port == 'Chromium':
                exclude = "*Chromium*.cpp"
            exclude_patterns.append(exclude)
    
    global uses
    for use in uses:
        if thisport not in port_uses or not use in port_uses[thisport]:
            exclude_patterns.append("*%s.cpp" % use)
            
    return exclude_patterns

def get_config():
    waf_configname = config.upper().strip()
    if building_on_win32:
        isReleaseCRT = (config == 'Release')
        if build_port == 'wx':
            if Options.options.wxpython:
                isReleaseCRT = True

        if isReleaseCRT:
            waf_configname = waf_configname + ' CRT_MULTITHREADED_DLL'
        else:
            waf_configname = waf_configname + ' CRT_MULTITHREADED_DLL_DBG'

    return waf_configname

create_hash_table = wk_root + "/Source/JavaScriptCore/create_hash_table"
if building_on_win32:
    create_hash_table = get_output('cygpath --unix "%s"' % create_hash_table)
os.environ['CREATE_HASH_TABLE'] = create_hash_table

feature_defines = []

msvc_version = 'msvc2008'

msvclibs_dir = os.path.join(wklibs_dir, msvc_version, 'win')


def get_path_to_wxconfig():
    if 'WX_CONFIG' in os.environ:
        return os.environ['WX_CONFIG']
    else:
        return 'wx-config'


def common_set_options(opt):
    """
    Initialize common options provided to the user.
    """
    opt.tool_options('compiler_cxx')
    opt.tool_options('compiler_cc')
    opt.tool_options('python')

    opt.add_option('--port', action='store', default=default_port, help='Which WebKit port to build.')
    opt.add_option('--wxpython', action='store_true', default=False, help='Create the wxPython bindings.')
    opt.add_option('--wx-compiler-prefix', action='store', default='vc',
                   help='Specify a different compiler prefix (do this if you used COMPILER_PREFIX when building wx itself)')
    opt.add_option('--macosx-version', action='store', default='', help="Version of OS X to build for.")
    opt.add_option('--msvc-version', action='store', default='', help="MSVC version to use to build. Use 8 for 2005, 9 for 2008")
    opt.add_option('--mac_universal_binary', action='store_true', default=False, help='Build Mac as universal (i386, x86_64, ppc) binary.')
    opt.add_option('--mac_archs', action='store', default='', help='Comma separated list of architectures (i386, x86_64, ppc) to build on Mac.')
    opt.add_option('--cairo', action='store_true', default=sys.platform.startswith('linux'), help='Use cairo for drawing (experimental for Win/Mac')

def common_configure(conf):
    """
    Configuration used by all targets, called from the target's configure() step.
    """

    conf.env['MSVC_TARGETS'] = ['x86']

    build_port = Options.options.port

    feature_defines = ['ENABLE_DATABASE', 'ENABLE_SQL_DATABASE', 'ENABLE_XSLT', 'ENABLE_JAVASCRIPT_DEBUGGER',
                    'ENABLE_SVG', 'ENABLE_FILTERS', 'ENABLE_SVG_FONTS', 'ENABLE_INSPECTOR', 'ENABLE_WORKERS',
                    'BUILDING_%s' % build_port.upper()]

    conf.env["FEATURE_DEFINES"] = ' '.join(feature_defines)

    if Options.options.msvc_version and Options.options.msvc_version != '':
        conf.env['MSVC_VERSIONS'] = ['msvc %s.0' % Options.options.msvc_version]
    else:
        conf.env['MSVC_VERSIONS'] = ['msvc 9.0', 'msvc 8.0']

    if sys.platform.startswith('cygwin'):
        print "ERROR: You must use the Win32 Python from python.org, not Cygwin Python, when building on Windows."
        sys.exit(1)

    conf.check_tool('compiler_cxx')
    conf.check_tool('compiler_cc')

    if sys.platform.startswith('darwin'):
        conf.check_tool('osx')

    global msvc_version
    global msvclibs_dir

    libprefix = ''

    if Options.options.cairo and build_port == 'wx':
        if building_on_win32 and not "CAIRO_ROOT" in os.environ:
            Logs.error("To build with Cairo on Windows, you must set the CAIRO_ROOT environment variable.")
            sys.exit(1)

    if building_on_win32:
        libprefix = 'lib'

        found = conf.get_msvc_versions()
        found_versions = []
        for version in found:
            found_versions.append(version[0])

        if 'msvc 9.0' in conf.env['MSVC_VERSIONS'] and 'msvc 9.0' in found_versions:
            msvc_version = 'msvc2008'
        elif 'msvc 8.0' in conf.env['MSVC_VERSIONS'] and 'msvc 8.0' in found_versions:
            msvc_version = 'msvc2005'

        msvclibs_dir = os.path.join(wklibs_dir, msvc_version, 'win')

        # Disable several warnings which occur many times during the build.
        # Some of them are harmless (4099, 4344, 4396, 4800) and working around
        # them in WebKit code is probably just not worth it. We can simply do
        # nothing about the others (4503). A couple are possibly valid but
        # there are just too many of them in the code so fixing them is
        # impossible in practice and just results in tons of distracting output
        # (4244, 4291). Finally 4996 is actively harmful as it is given for
        # just about any use of standard C/C++ library facilities.
        conf.env.append_value('CXXFLAGS', [
            '/wd4099',  # type name first seen using 'struct' now seen using 'class'
            '/wd4244',  # conversion from 'xxx' to 'yyy', possible loss of data:
            '/wd4291',  # no matching operator delete found (for placement new)
            '/wd4344',  # behaviour change in template deduction
            '/wd4396',  # inline can't be used in friend declaration
            '/wd4503',  # decorated name length exceeded, name was truncated
            '/wd4800',  # forcing value to bool 'true' or 'false'
            '/wd4996',  # deprecated function
        ])

        # This one also occurs in C code, so disable it there as well.
        conf.env.append_value('CCFLAGS', ['/wd4996'])

    if build_port == "wx":
        update_wx_deps(conf, wk_root, msvc_version)

        conf.env.append_value('CXXDEFINES', ['BUILDING_WX__=1'])

        if building_on_win32:
            conf.env.append_value('LIBPATH', os.path.join(msvclibs_dir, 'lib'))
            # wx settings
            global config
            is_debug = (config == 'Debug')            
            # generate debug symbols even in release mode, as it helps debugging.
            if not is_debug:
                conf.env.append_value('CXXFLAGS', '/Zi')
                conf.env.append_value('LINKFLAGS', '/debug')
            wxdefines, wxincludes, wxlibs, wxlibpaths = get_wxmsw_settings(wx_root, shared=True, unicode=True, debug=is_debug, wxPython=Options.options.wxpython)
            conf.env['CXXDEFINES_WX'] = wxdefines
            conf.env['CPPPATH_WX'] = wxincludes
            conf.env['LIB_WX'] = wxlibs
            conf.env['LIBPATH_WX'] = wxlibpaths
            if Options.options.cairo and 'CAIRO_ROOT' in os.environ:
                conf.env.append_value('CPPPATH_WX', [os.path.join(os.environ['CAIRO_ROOT'], 'include', 'cairo')])
                conf.env.append_value('LIBPATH_WX', [os.path.join(os.environ['CAIRO_ROOT'], 'lib')])
                conf.env.append_value('LIB_WX', ['cairo'])
        else:
            conf.check_cfg(path=get_path_to_wxconfig(), args='--cxxflags --libs', package='', uselib_store='WX', mandatory=True)

    if sys.platform.startswith('darwin'):
        conf.env['LIB_ICU'] = ['icucore']
        global port_uses
        port_uses[build_port].append('CF')
        
        conf.env.append_value('CPPPATH', wklibs_dir)
        conf.env.append_value('LIBPATH', wklibs_dir)

        min_version = None

        mac_target = 'MACOSX_DEPLOYMENT_TARGET'
        if Options.options.macosx_version != '':
            min_version = Options.options.macosx_version

        # WebKit only supports 10.4+, but ppc systems often set this to earlier systems
        if not min_version:
            min_version = commands.getoutput('sw_vers -productVersion')[:4]
            if min_version in ['10.1', '10.2', '10.3']:
                min_version = '10.4'

        sdk_version = min_version
        if min_version == "10.4":
            sdk_version += "u"
            conf.env.append_value('LIB_WKINTERFACE', ['WebKitSystemInterfaceTiger'])
        elif min_version == "10.7":
            conf.env.append_value('LIB_WKINTERFACE', ['WebKitSystemInterfaceLion'])
        else:
            # NOTE: There is a WebKitSystemInterfaceSnowLeopard, but when we use that
            # on 10.6, we get a strange missing symbol error, and this library seems to
            # work fine for wx's purposes.
            conf.env.append_value('LIB_WKINTERFACE', ['WebKitSystemInterfaceLeopard'])
            conf.env.append_value('LINKFLAGS', ['-framework', 'QuartzCore'])
        
        # match WebKit Mac's default here unless we're building on platforms that won't support 64-bit.
        archs = []
        
        if Options.options.mac_archs != '':
            arch_list = Options.options.mac_archs.replace("'", '').replace('"', '').split(",")
            for arch in arch_list:
                if arch.strip() != '':
                    archs.append(arch.strip())
        elif Options.options.mac_universal_binary:
            archs = ['i386', 'x86_64', 'ppc']
        else:
            is_cocoa = "__WXOSX_COCOA__" in conf.env["CXXDEFINES_WX"]
            if min_version == "10.4" or not is_cocoa:
                archs = ["i386"]
            else:
                global arch
                archs = [arch]

        sdkroot = '/Developer/SDKs/MacOSX%s.sdk' % sdk_version
        if not os.path.exists(sdkroot):
            Logs.error("Cannot find the specified SDK needed to build: %r" % sdkroot)
            sys.exit(1)
        sdkflags = ['-isysroot', sdkroot]
        for arch in archs:
            sdkflags.extend(['-arch', arch])

        conf.env.append_value('CPPFLAGS', sdkflags)
        conf.env.append_value('LINKFLAGS', sdkflags)

        conf.env.append_value('LINKFLAGS', ['-framework', 'Security'])

        conf.env.append_value('CPPPATH_SQLITE3', [os.path.join(wklibs_dir, 'WebCoreSQLite3')])
        conf.env.append_value('LIB_SQLITE3', ['WebCoreSQLite3'])

    # NOTE: The order here is important, because python sets the MACOSX_DEPLOYMENT_TARGET to
    # 10.3 even on intel. So we must first set the SDK and arch flags, then load Python's config,
    # and finally override the value Python set for MACOSX_DEPLOYMENT_TARGET
    if Options.options.wxpython:
        conf.check_tool('python')
        conf.check_python_headers()

    if sys.platform.startswith('darwin'):
        os.environ[mac_target] = conf.env[mac_target] = min_version

    conf.env.append_value('CXXDEFINES', feature_defines)
    if config == 'Release':
        conf.env.append_value('CPPDEFINES', 'NDEBUG')

    if building_on_win32:
        conf.env.append_value('CPPPATH', [
            os.path.join(jscore_dir, 'os-win32'),
            os.path.join(msvclibs_dir, 'include'),
            os.path.join(msvclibs_dir, 'include', 'pthreads'),
            os.path.join(msvclibs_dir, 'lib'),
            ])

        conf.env.append_value('LIB', ['libpng', 'libjpeg', 'pthreadVC2'])
        # common win libs
        conf.env.append_value('LIB', [
            'kernel32', 'user32', 'gdi32', 'comdlg32', 'winspool', 'winmm',
            'shell32', 'shlwapi', 'comctl32', 'ole32', 'oleaut32', 'uuid', 'advapi32',
            'wsock32', 'gdiplus', 'usp10', 'version'])

        conf.env['LIB_ICU'] = ['icudt', 'icule', 'iculx', 'icuuc', 'icuin', 'icuio', 'icutu']

        #curl
        conf.env['LIB_CURL'] = ['libcurl']

        #sqlite3
        conf.env['CPPPATH_SQLITE3'] = [os.path.join(msvclibs_dir, 'include', 'SQLite')]
        conf.env['LIB_SQLITE3'] = ['sqlite3']

        #libxml2
        conf.env['LIB_XML'] = ['libxml2']

        #libxslt
        conf.env['LIB_XSLT'] = ['libxslt']
    else:
        conf.env.append_value('CXXFLAGS', ['-fvisibility=hidden'])
        if build_port == 'wx':
            port_uses['wx'].append('PTHREADS')
            conf.env.append_value('LIB', ['jpeg', 'png', 'pthread'])
            conf.env.append_value('LIBPATH', os.path.join(wklibs_dir, 'unix', 'lib'))
            conf.env.append_value('CPPPATH', os.path.join(wklibs_dir, 'unix', 'include'))
            conf.env.append_value('CXXFLAGS', ['-fPIC', '-DPIC'])
            conf.env.append_value('CXXFLAGS', ['-g'])
        conf.check_cfg(msg='Checking for libxslt', path='xslt-config', args='--cflags --libs', package='', uselib_store='XSLT', mandatory=True)
        conf.check_cfg(path='xml2-config', args='--cflags --libs', package='', uselib_store='XML', mandatory=True)
        if sys.platform.startswith('darwin') and min_version and min_version == '10.4':
            conf.check_cfg(path=os.path.join(wklibs_dir, 'unix', 'bin', 'curl-config'), args='--cflags --libs', package='', uselib_store='CURL', mandatory=True)
        else:
            conf.check_cfg(path='curl-config', args='--cflags --libs', package='', uselib_store='CURL', mandatory=True)

        if not sys.platform.startswith('darwin'):
            # this is needed to keep ld from hitting the 4gb process limit under Linux/Unix.
            conf.env.append_value('LINKFLAGS', ['-Wl,--no-keep-memory'])
            conf.check_cfg(package='cairo', args='--cflags --libs', uselib_store='WX', mandatory=True)
            conf.check_cfg(package='pango', args='--cflags --libs', uselib_store='WX', mandatory=True)
            conf.check_cfg(package='gtk+-2.0', args='--cflags --libs', uselib_store='WX', mandatory=True)
            conf.check_cfg(package='sqlite3', args='--cflags --libs', uselib_store='SQLITE3', mandatory=True)
            conf.check_cfg(path='icu-config', args='--cflags --ldflags', package='', uselib_store='ICU', mandatory=True)

    if build_port in port_uses:
        for use in port_uses[build_port]:
            conf.env.append_value('CXXDEFINES', ['WTF_USE_%s' % use])
