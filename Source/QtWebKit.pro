# -------------------------------------------------------------------
# Root project file for QtWebKit
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

api.file = api.pri
SUBDIRS += api

!no_webkit2 {
    webprocess.file = WebKit2/WebProcess.pro
    SUBDIRS += webprocess
}

include(WebKit/qt/docs/docs.pri)

SUBDIRS += WebKit/qt/declarative
haveQt(5) {
    !no_webkit2 {
        SUBDIRS += WebKit/qt/declarative/private
    }
}

tests.file = tests.pri
SUBDIRS += tests

examples.file = WebKit/qt/examples/examples.pro
examples.CONFIG += no_default_target
examples.makefile = Makefile
SUBDIRS += examples

haveQt(4):!build_pass {
    # Use our own copy of syncqt from Qt 4.8 to generate forwarding headers
    syncqt = $$toSystemPath($${ROOT_WEBKIT_DIR}/Tools/qmake/syncqt-4.8)
    command = $$syncqt
    win32-msvc*: command = $$command -windows

    outdir = $$toSystemPath($${ROOT_BUILD_DIR})

    # The module root has to be the same as directory of the pro-file that generates
    # the install rules (api.pri), otherwise the relative paths in the generated
    # headers.pri will be incorrect.
    module_rootdir = $$toSystemPath($${_PRO_FILE_PWD_})

    module = $${TARGET}$${DIRLIST_SEPARATOR}$${module_rootdir}$${DIRLIST_SEPARATOR}$$toSystemPath(WebKit/qt/Api)
    fwheader_generator.commands = perl $${command} -outdir $${outdir} -separate-module $${module}
    fwheader_generator.depends = $${syncqt}

    variables = $$computeSubdirVariables(api)

    api_qmake.target = $$eval($${variables}.target)-qmake_all
    api_qmake.depends = fwheader_generator

    api_makefile.target = $$eval($${variables}.makefile)
    api_makefile.depends = fwheader_generator

    QMAKE_EXTRA_TARGETS += fwheader_generator api_qmake api_makefile
}
