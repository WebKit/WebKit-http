/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWebChannel module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

import QtWebChannel 1.0

Item {
    id: test
    signal barCalled(var arg)
    signal clientInitializedCalled(var arg)

    QtObject {
        id: testObject
        WebChannel.id: "testObject"

        property var foo: 42

        function clientInitialized(arg)
        {
            clientInitializedCalled(arg);
        }

        function bar(arg) {
            barCalled(arg);
        }

        signal runTest(var foo)
    }

    TestWebView {
        id: webView
        experimental.webChannel.registeredObjects: [testObject]
        experimental.preferences.developerExtrasEnabled: true
    }

    SignalSpy {
        id: initializedSpy
        target: test
        signalName: "clientInitializedCalled"
    }

    SignalSpy {
        id: barSpy
        target: test
        signalName: "barCalled"
    }

    TestCase {
        name: "WebViewWebChannel"
        property url testUrl: Qt.resolvedUrl("../common/webchannel.html")

        function init() {
            initializedSpy.clear();
            barSpy.clear();
        }

        function test_basic() {
            webView.url = testUrl;
            verify(webView.waitForLoadSucceeded());

            initializedSpy.wait();
            compare(initializedSpy.signalArguments.length, 1);
            compare(initializedSpy.signalArguments[0][0], 42);

            var newValue = "roundtrip";
            testObject.runTest(newValue);
            barSpy.wait();
            compare(barSpy.signalArguments.length, 1);
            compare(barSpy.signalArguments[0][0], newValue);

            compare(testObject.foo, newValue);
        }
    }
}
