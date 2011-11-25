import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0

Item {
    DesktopWebView {
        id: webView
        property variant lastMessage
        preferences.navigatorQtObjectEnabled: true
        onMessageReceived: {
            lastMessage = message
        }
    }

    DesktopWebView {
        id: otherWebView
        property variant lastMessage
        preferences.navigatorQtObjectEnabled: true
        onMessageReceived: {
            lastMessage = message
        }
    }

    DesktopWebView {
        id: disabledWebView
        property bool receivedMessage
        preferences.navigatorQtObjectEnabled: false
        onMessageReceived: {
            receivedMessage = true
        }
    }

    SignalSpy {
        id: loadSpy
        target: webView
        signalName: "loadSucceeded"
    }

    SignalSpy {
        id: messageSpy
        target: webView
        signalName: "messageReceived"
    }

    SignalSpy {
        id: otherLoadSpy
        target: otherWebView
        signalName: "loadSucceeded"
    }

    SignalSpy {
        id: otherMessageSpy
        target: otherWebView
        signalName: "messageReceived"
    }

    SignalSpy {
        id: disabledWebViewLoadSpy
        target: disabledWebView
        signalName: "loadSucceeded"
    }

    TestCase {
        name: "DesktopWebViewMessaging"
        property url testUrl: Qt.resolvedUrl("../common/messaging.html")

        function init() {
            loadSpy.clear()
            messageSpy.clear()
            webView.lastMessage = null
            otherLoadSpy.clear()
            otherMessageSpy.clear()
            otherWebView.lastMessage = null
        }

        function test_basic() {
            webView.load(testUrl)
            loadSpy.wait()
            webView.postMessage("HELLO")
            messageSpy.wait()
            compare(webView.lastMessage.data, "OLLEH")
            compare(webView.lastMessage.origin.toString(), testUrl.toString())
        }

        function test_twoWebViews() {
            webView.load(testUrl)
            otherWebView.load(testUrl)
            loadSpy.wait()
            otherLoadSpy.wait()
            webView.postMessage("FIRST")
            otherWebView.postMessage("SECOND")
            messageSpy.wait()
            otherMessageSpy.wait()
            compare(webView.lastMessage.data, "TSRIF")
            compare(otherWebView.lastMessage.data, "DNOCES")
        }

        function test_disabled() {
            disabledWebView.load(testUrl)
            verify(!disabledWebView.preferences.navigatorQtObjectEnabled)
            disabledWebViewLoadSpy.wait()
            disabledWebView.postMessage("HI")
            wait(1000)
            verify(!disabledWebView.receivedMessage)
        }
    }
}
