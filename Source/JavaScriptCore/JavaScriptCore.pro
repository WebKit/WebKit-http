# -------------------------------------------------------------------
# Main project file for JavaScriptSource
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

derived_sources.file = DerivedSources.pri
target.file = Target.pri

SUBDIRS += derived_sources target

addStrictSubdirOrderBetween(derived_sources, target)

jsc.file = jsc.pro
jsc.makefile = Makefile.jsc
SUBDIRS += jsc
