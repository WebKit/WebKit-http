/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
var context = {};
(function () {
    Preferences.ignoreWhitespace = false;
    Preferences.samplingCPUProfiler = true;
    Preferences.debuggerAlwaysEnabled = true;
    Preferences.profilerAlwaysEnabled = true;
    Preferences.canEditScriptSource = false;
    Preferences.onlineDetectionEnabled = false;
    Preferences.nativeInstrumentationEnabled = true;
    // FIXME: Turn this to whatever the value of --enable-file-system for chrome.
    Preferences.fileSystemEnabled = false;
    Preferences.canClearCacheAndCookies = true;
    Preferences.showCookiesTab = true;
})();
InspectorFrontendHost.copyText = function(tmp) {
    var encoded = encodeURI(tmp);
    var text = 'data:text/plain;charset=utf-8,' + encoded;
    window.open(text, "_blank");
}
