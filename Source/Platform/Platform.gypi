#
# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#         * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#         * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#         * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

{
    'variables': {
        'platform_files': [
            'chromium/public/Platform.h',
            'chromium/public/WebAnimation.h',
            'chromium/public/WebAnimationCurve.h',
            'chromium/public/WebAnimationDelegate.h',
            'chromium/public/WebAudioBus.h',
            'chromium/public/WebAudioDevice.h',
            'chromium/public/WebBlobData.h',
            'chromium/public/WebBlobRegistry.h',
            'chromium/public/WebCString.h',
            'chromium/public/WebCanvas.h',
            'chromium/public/WebClipboard.h',
            'chromium/public/WebColor.h',
            'chromium/public/WebCommon.h',
            'chromium/public/WebCompositor.h',
            'chromium/public/WebCompositorSupport.h',
            'chromium/public/WebCompositorSoftwareOutputDevice.h',
            'chromium/public/WebCompositorOutputSurface.h',
            'chromium/public/WebCompositorOutputSurfaceClient.h',
            'chromium/public/WebContentLayer.h',
            'chromium/public/WebContentLayerClient.h',
            'chromium/public/WebCookie.h',
            'chromium/public/WebCookieJar.h',
            'chromium/public/WebData.h',
            'chromium/public/WebDelegatedRendererLayer.h',
            'chromium/public/WebDragData.h',
            'chromium/public/WebExternalTextureLayer.h',
            'chromium/public/WebExternalTextureLayerClient.h',
            'chromium/public/WebFileInfo.h',
            'chromium/public/WebFileSystem.h',
            'chromium/public/WebFileUtilities.h',
            'chromium/public/WebFilterOperation.h',
            'chromium/public/WebFilterOperations.h',
            'chromium/public/WebFlingAnimator.h',
            'chromium/public/WebFloatAnimationCurve.h',
            'chromium/public/WebFloatKeyframe.h',
            'chromium/public/WebFloatPoint.h',
            'chromium/public/WebFloatQuad.h',
            'chromium/public/WebFloatRect.h',
            'chromium/public/WebGamepad.h',
            'chromium/public/WebGamepads.h',
            'chromium/public/WebGraphicsContext3D.h',
            'chromium/public/WebHTTPBody.h',
            'chromium/public/WebHTTPHeaderVisitor.h',
            'chromium/public/WebHTTPLoadInfo.h',
            'chromium/public/WebICECandidateDescriptor.h',
            'chromium/public/WebICEOptions.h',
            'chromium/public/WebImage.h',
            'chromium/public/WebImageLayer.h',
            'chromium/public/WebIOSurfaceLayer.h',
            'chromium/public/WebLayer.h',
            'chromium/public/WebLayerScrollClient.h',
            'chromium/public/WebLayerTreeView.h',
            'chromium/public/WebLayerTreeViewClient.h',
            'chromium/public/WebLocalizedString.h',
            'chromium/public/WebMediaConstraints.h',
            'chromium/public/WebMediaHints.h',
            'chromium/public/WebMediaStreamCenter.h',
            'chromium/public/WebMediaStreamCenterClient.h',
            'chromium/public/WebMediaStreamComponent.h',
            'chromium/public/WebMediaStreamDescriptor.h',
            'chromium/public/WebMediaStreamSource.h',
            'chromium/public/WebMediaStreamSourcesRequest.h',
            'chromium/public/WebMessagePortChannel.h',
            'chromium/public/WebMessagePortChannelClient.h',
            'chromium/public/WebMimeRegistry.h',
            'chromium/public/WebNonCopyable.h',
            'chromium/public/WebPeerConnection00Handler.h',
            'chromium/public/WebPeerConnection00HandlerClient.h',
            'chromium/public/WebPeerConnectionHandler.h',
            'chromium/public/WebPeerConnectionHandlerClient.h',
            'chromium/public/WebPoint.h',
            'chromium/public/WebPrerender.h',
            'chromium/public/WebPrerenderingSupport.h',
            'chromium/public/WebPrivateOwnPtr.h',
            'chromium/public/WebPrivatePtr.h',
            'chromium/public/WebRTCConfiguration.h',
            'chromium/public/WebRTCICECandidate.h',
            'chromium/public/WebRTCPeerConnectionHandler.h',
            'chromium/public/WebRTCPeerConnectionHandlerClient.h',
            'chromium/public/WebRTCSessionDescription.h',
            'chromium/public/WebRTCSessionDescriptionRequest.h',
            'chromium/public/WebRTCVoidRequest.h',
            'chromium/public/WebRect.h',
            'chromium/public/WebReferrerPolicy.h',
            'chromium/public/WebRenderingStats.h',
            'chromium/public/WebScrollbar.h',
            'chromium/public/WebScrollbarLayer.h',
            'chromium/public/WebScrollbarThemeGeometry.h',
            'chromium/public/WebScrollbarThemePainter.h',
            'chromium/public/WebSessionDescriptionDescriptor.h',
            'chromium/public/WebSharedGraphicsContext3D.h',
            'chromium/public/WebSize.h',
            'chromium/public/WebSocketStreamError.h',
            'chromium/public/WebSocketStreamHandle.h',
            'chromium/public/WebSocketStreamHandleClient.h',
            'chromium/public/WebSolidColorLayer.h',
            'chromium/public/WebStorageArea.h',
            'chromium/public/WebStorageNamespace.h',
            'chromium/public/WebString.h',
            'chromium/public/WebThread.h',
            'chromium/public/WebThreadSafeData.h',
            'chromium/public/WebTransformationMatrix.h',
            'chromium/public/WebTransformAnimationCurve.h',
            'chromium/public/WebTransformKeyframe.h',
            'chromium/public/WebTransformOperations.h',
            'chromium/public/WebPrerender.h',
            'chromium/public/WebURL.h',
            'chromium/public/WebURLError.h',
            'chromium/public/WebURLLoadTiming.h',
            'chromium/public/WebURLLoader.h',
            'chromium/public/WebURLLoaderClient.h',
            'chromium/public/WebURLRequest.h',
            'chromium/public/WebURLResponse.h',
            'chromium/public/WebVector.h',
            'chromium/public/WebVideoLayer.h',
            'chromium/public/WebVideoFrame.h',
            'chromium/public/WebVideoFrameProvider.h',
            'chromium/public/WebWorkerRunLoop.h',
            'chromium/public/android/WebSandboxSupport.h',
            'chromium/public/android/WebThemeEngine.h',
            'chromium/public/linux/WebFontInfo.h',
            'chromium/public/linux/WebFontRenderStyle.h',
            'chromium/public/linux/WebSandboxSupport.h',
            'chromium/public/linux/WebThemeEngine.h',
            'chromium/public/mac/WebSandboxSupport.h',
            'chromium/public/mac/WebThemeEngine.h',
            'chromium/public/win/WebSandboxSupport.h',
            'chromium/public/win/WebThemeEngine.h',
            'chromium/src/Platform.cpp',
            'chromium/src/WebCString.cpp',
            'chromium/src/WebFilterOperation.cpp',
            'chromium/src/WebFilterOperations.cpp',
            'chromium/src/WebFloatQuad.cpp',
            'chromium/src/WebPrerenderingSupport.cpp',
            'chromium/src/WebString.cpp',
        ]
    }
}
