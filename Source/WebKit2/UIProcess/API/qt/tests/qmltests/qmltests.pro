TEMPLATE = app
TARGET = tst_qmltests
CONFIG += warn_on testcase
SOURCES += tst_qmltests.cpp

QT += declarative qmltest

# QML files tested are the ones in WebKit source repository.
DEFINES += QUICK_TEST_SOURCE_DIR=\"\\\"$$PWD\\\"\"

OTHER_FILES += \
    DesktopWebView/tst_properties.qml \
    DesktopWebView/tst_navigationPolicyForUrl.qml \
    TouchWebView/tst_properties.qml \
    TouchWebView/tst_loadZeroSizeView.qml \
    TouchWebView/tst_loadNegativeSizeView.qml \
    TouchWebView/tst_loadProgress.qml

