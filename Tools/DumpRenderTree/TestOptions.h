/*
 * Copyright (C) 2016-2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <string>

struct TestOptions {
    bool enableAttachmentElement { false };
    bool enableWebAnimationsCSSIntegration { true };
    bool useAcceleratedDrawing { false };
    bool enableIntersectionObserver { false };
    bool useEphemeralSession { false };
    bool enableBackForwardCache { false };
    bool enableMenuItemElement { false };
    bool enableKeygenElement { false };
    bool enableModernMediaControls { true };
    bool enablePointerLock { false };
    bool enableDragDestinationActionLoad { false };
    bool layerBackedWebView { false };
    bool enableIsSecureContextAttribute { true };
    bool enableInspectorAdditions { false };
    bool dumpJSConsoleLogInStdErr { false };
    bool allowCrossOriginSubresourcesToAskForCredentials { false };
    bool enableColorFilter { false };
    bool enableSelectionAcrossShadowBoundaries { true };
    bool enableWebGPU { false };
    bool enableCSSLogical { false };
    bool enableLineHeightUnits { false };
    bool adClickAttributionEnabled { false };
    bool enableResizeObserver { false };
    bool enableCSSOMViewSmoothScrolling { false };
    bool enableCoreMathML { false };
    bool enableRequestIdleCallback { false };
    bool enableAsyncClipboardAPI { false };
    bool layoutFormattingContextIntegrationEnabled { true };
    bool enableAspectRatioOfImgFromWidthAndHeight { false };
    bool enableWebSQL { true };
    bool allowTopNavigationToDataURLs { true };
    std::string jscOptions;
    std::string additionalSupportedImageTypes;

    TestOptions(const std::string& pathOrURL, const std::string& absolutePath);
    bool webViewIsCompatibleWithOptions(const TestOptions&) const;
};
