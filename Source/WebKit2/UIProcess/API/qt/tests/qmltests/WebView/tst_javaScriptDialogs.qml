import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 3.0

WebView {
    id: webView

    property bool modelMessageEqualsMessage: false
    property string messageFromAlertDialog: ""
    property int confirmCount: 0
    property int promptCount: 0

    experimental.alertDialog: Item {
        Timer {
            running: true
            interval: 1
            onTriggered: {
                // Testing both attached property and id defined in the Component context.
                parent.WebView.view.messageFromAlertDialog = message
                webView.modelMessageEqualsMessage = Boolean(model.message == message)
                model.dismiss()
            }
        }
    }

    experimental.confirmDialog: Item {
        Timer {
            running: true
            interval: 1
            onTriggered: {
                parent.WebView.view.confirmCount += 1
                if (message == "ACCEPT")
                    model.accept()
                else
                    model.reject()
            }
        }
    }

    experimental.promptDialog: Item {
        Timer {
            running: true
            interval: 1
            onTriggered: {
                parent.WebView.view.promptCount += 1
                if (message == "REJECT")
                    model.reject()
                else {
                    var reversedDefaultValue = defaultValue.split("").reverse().join("")
                    model.accept(reversedDefaultValue)
                }
            }
        }
    }

    SignalSpy {
        id: loadSpy
        target: webView
        signalName: "loadSucceeded"
    }

    TestCase {
        id: test
        name: "WebViewJavaScriptDialogs"

        function init() {
            webView.modelMessageEqualsMessage = false
            webView.messageFromAlertDialog = ""
            webView.confirmCount = 0
            webView.promptCount = 0
            loadSpy.clear()
        }

        function test_alert() {
            webView.load(Qt.resolvedUrl("../common/alert.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.messageFromAlertDialog, "Hello Qt")
            verify(webView.modelMessageEqualsMessage)
        }

        function test_alertWithoutDialog() {
            skip("Setting experimental properties from JS code isn't working")
            webView.experimental.alertDialog = null
            webView.load(Qt.resolvedUrl("../common/alert.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.messageFromAlertDialog, "")
        }

        function test_confirm() {
            webView.load(Qt.resolvedUrl("../common/confirm.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.confirmCount, 2)
            compare(webView.title, "ACCEPTED REJECTED")
        }

        function test_confirmWithoutDialog() {
            skip("Setting experimental properties from JS code isn't working")
            webView.experimental.confirmDialog = null
            webView.load(Qt.resolvedUrl("../common/confirm.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.confirmCount, 0)
            compare(webView.title, "ACCEPTED ACCEPTED")
        }

        function test_prompt() {
            webView.load(Qt.resolvedUrl("../common/prompt.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.promptCount, 2)
            compare(webView.title, "tQ olleH")
        }

        function test_promptWithoutDialog() {
            skip("Setting experimental properties from JS code isn't working")
            webView.experimental.promptDialog = null
            webView.load(Qt.resolvedUrl("../common/prompt.html"))
            loadSpy.wait()
            compare(loadSpy.count, 1)
            compare(webView.promptCount, 0)
            compare(webView.title, "FAIL")
        }
    }
}
