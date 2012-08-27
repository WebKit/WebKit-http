import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import Test 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 480
        height: 720

        property variant result

        property variant content: "data:text/html," +
            "<head>" +
            "    <meta name='viewport' content='width=device-width'>" +
            "</head>" +
            "<body>" +
            "    <div id='target' " +
            "         style='position:absolute; left:20; top:20; width:200; height:200;'>" +
            "    </div>" +
            "</body>"

        signal resultReceived
    }

    SignalSpy {
        id: resultSpy
        target: webView
        signalName: "resultReceived"
    }

    SignalSpy {
        id: scaleSpy
        target: webView.experimental.test
        signalName: "contentsScaleCommitted"
    }

    TestCase {
        name: "DoubleTapToZoom"

        property variant test: webView.experimental.test

        function init() {
            resultSpy.clear()
            scaleSpy.clear()
        }

        function documentSize() {
            resultSpy.clear();
            var result;

             webView.experimental.evaluateJavaScript(
                "window.innerWidth + 'x' + window.innerHeight",
                function(size) { webView.resultReceived(); result = size });
            resultSpy.wait();
            return result;
        }

        function elementRect(id) {
            resultSpy.clear();
            var result;

             webView.experimental.evaluateJavaScript(
                "JSON.stringify(document.getElementById('" + id + "').getBoundingClientRect());",
                function(rect) { webView.resultReceived(); result = JSON.parse(rect); });
            resultSpy.wait();
            return result;
        }

        function doubleTapAtPoint(x, y) {
            scaleSpy.clear()
            test.touchDoubleTap(webView, x, y)
            scaleSpy.wait()
        }

        function test_basic() {
            webView.url = webView.content
            verify(webView.waitForLoadSucceeded())

            compare(documentSize(), "480x720")

            compare(test.contentsScale, 1.0)

            var rect = elementRect("target");
            var newScale = webView.width / (rect.width + 2 * 10) // inflated by 10px
            doubleTapAtPoint(rect.left + rect.height / 2, rect.top + rect.width / 2)

            compare(test.contentsScale, newScale)
        }
    }
}
