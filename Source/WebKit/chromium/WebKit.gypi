#
# Copyright (C) 2010 Google Inc. All rights reserved.
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
        # List of DevTools source files, ordered by dependencies. It is used both
        # for copying them to resource dir, and for generating 'devtools.html' file.
        'devtools_js_files': [
            'src/js/DevTools.js',
            'src/js/DevToolsExtensionAPI.js',
            'src/js/Tests.js',
        ],
        'devtools_css_files': [
            'src/js/devTools.css',
        ],
        'devtools_extension_api_files': [
            '../../WebCore/inspector/front-end/ExtensionAPI.js',
            'src/js/DevToolsExtensionAPI.js'
        ],
        'devtools_image_files': [
            'src/js/Images/segmentChromium.png',
            'src/js/Images/segmentHoverChromium.png',
            'src/js/Images/segmentHoverEndChromium.png',
            'src/js/Images/segmentSelectedChromium.png',
            'src/js/Images/segmentSelectedEndChromium.png',
        ],
        'webkit_unittest_files': [
            'tests/AnimationTranslationUtilTest.cpp',
            'tests/ArenaTestHelpers.h',
            'tests/AssociatedURLLoaderTest.cpp',
            'tests/Canvas2DLayerBridgeTest.cpp',
            'tests/Canvas2DLayerManagerTest.cpp',
            'tests/ClipboardChromiumTest.cpp',
            'tests/DateTimeFormatTest.cpp',
            'tests/DecimalTest.cpp',
            'tests/DragImageTest.cpp',
            'tests/EventListenerTest.cpp',
            'tests/FilterOperationsTest.cpp',
            'tests/FrameLoaderClientImplTest.cpp',
            'tests/FrameTestHelpers.cpp',
            'tests/FrameTestHelpers.h',
            'tests/GraphicsLayerChromiumTest.cpp',
            'tests/IDBAbortOnCorruptTest.cpp',
            'tests/IDBBindingUtilitiesTest.cpp',
            'tests/IDBDatabaseBackendTest.cpp',
            'tests/IDBFakeBackingStore.h',
            'tests/IDBKeyPathTest.cpp',
            'tests/IDBLevelDBCodingTest.cpp',
            'tests/IDBRequestTest.cpp',
            'tests/ImageLayerChromiumTest.cpp',
            'tests/KeyboardTest.cpp',
            'tests/KURLTest.cpp',
            'tests/LevelDBTest.cpp',
            'tests/LinkHighlightTest.cpp',
            'tests/ListenerLeakTest.cpp',
            'tests/MemoryInfo.cpp',
            'tests/MemoryInstrumentationTest.cpp',
            'tests/OpaqueRectTrackingContentLayerDelegateTest.cpp',
            'tests/OpenTypeVerticalDataTest.cpp',
            'tests/PaintAggregatorTest.cpp',
            'tests/PlatformGestureCurveTest.cpp',
            'tests/PlatformContextSkiaTest.cpp',
            'tests/PODArenaTest.cpp',
            'tests/PODIntervalTreeTest.cpp',
            'tests/PODRedBlackTreeTest.cpp',
            'tests/PopupContainerTest.cpp',
            'tests/RegionTest.cpp',
            'tests/RenderTableCellTest.cpp',
            'tests/RenderTableRowTest.cpp',
            'tests/TilingDataTest.cpp',
            'tests/TreeTestHelpers.cpp',
            'tests/TreeTestHelpers.h',
            'tests/URLTestHelpers.cpp',
            'tests/URLTestHelpers.h',
            'tests/WebCompositorInitializer.h',
            'tests/WebCompositorInputHandlerImplTest.cpp',
            'tests/WebFrameTest.cpp',
            'tests/WebInputEventConversionTest.cpp',
            'tests/WebImageTest.cpp',
            'tests/WebMediaPlayerClientImplTest.cpp',
            'tests/WebPageNewSerializerTest.cpp',
            'tests/WebPageSerializerTest.cpp',
            'tests/WebSocketDeflaterTest.cpp',
            'tests/WebSocketExtensionDispatcherTest.cpp',
            'tests/WebURLRequestTest.cpp',
            'tests/WebURLResponseTest.cpp',
            'tests/WebViewTest.cpp',
        ],

        'webkit_compositor_unittest_files': [
            'tests/CCActiveAnimationTest.cpp',
            'tests/CCAnimationTestCommon.cpp',
            'tests/CCAnimationTestCommon.h',
            'tests/CCDamageTrackerTest.cpp',
            'tests/CCDelayBasedTimeSourceTest.cpp',
            'tests/CCDrawQuadTest.cpp',
            'tests/CCFrameRateControllerTest.cpp',
            'tests/CCGeometryTestUtils.cpp',
            'tests/CCGeometryTestUtils.h',
            'tests/CCHeadsUpDisplayTest.cpp',
            'tests/CCKeyframedAnimationCurveTest.cpp',
            'tests/CCLayerAnimationControllerTest.cpp',
            'tests/CCLayerImplTest.cpp',
            'tests/CCLayerIteratorTest.cpp',
            'tests/CCLayerQuadTest.cpp',
            'tests/CCLayerSorterTest.cpp',
            'tests/CCLayerTestCommon.cpp',
            'tests/CCLayerTestCommon.h',
            'tests/CCLayerTreeHostCommonTest.cpp',
            'tests/CCLayerTreeHostImplTest.cpp',
            'tests/CCLayerTreeHostTest.cpp',
            'tests/CCMathUtilTest.cpp',
            'tests/CCOcclusionTrackerTest.cpp',
            'tests/CCOcclusionTrackerTestCommon.h',
            'tests/CCPrioritizedTextureTest.cpp',
            'tests/CCQuadCullerTest.cpp',
            'tests/CCRenderPassTest.cpp',
            'tests/CCRenderSurfaceFiltersTest.cpp',
            'tests/CCRenderSurfaceTest.cpp',
            'tests/CCRendererGLTest.cpp',
            'tests/CCResourceProviderTest.cpp',
            'tests/CCSchedulerStateMachineTest.cpp',
            'tests/CCSchedulerTest.cpp',
            'tests/CCSchedulerTestCommon.h',
            'tests/CCScopedTextureTest.cpp',
            'tests/CCScrollbarAnimationControllerLinearFadeTest.cpp',
            'tests/CCSolidColorLayerImplTest.cpp',
            'tests/CCTestCommon.h',
            'tests/CCTextureUpdateControllerTest.cpp',
            'tests/CCThreadTaskTest.cpp',
            'tests/CCThreadedTest.cpp',
            'tests/CCThreadedTest.h',
            'tests/CCTiledLayerImplTest.cpp',
            'tests/CCTiledLayerTestCommon.cpp',
            'tests/CCTiledLayerTestCommon.h',
            'tests/CCTimerTest.cpp',
            'tests/CompositorFakeWebGraphicsContext3D.h',
            'tests/ContentLayerChromiumTest.cpp',
            'tests/FakeCCLayerTreeHostClient.h',
            'tests/FakeGraphicsContext3DTest.cpp',
            'tests/FakeWebCompositorOutputSurface.h',
            'tests/FakeWebGraphicsContext3D.h',
            'tests/FakeWebScrollbarThemeGeometry.h',
            'tests/FloatQuadTest.cpp',
            'tests/LayerChromiumTest.cpp',
            'tests/MockCCQuadCuller.h',
            'tests/ScrollbarLayerChromiumTest.cpp',
            'tests/TextureCopierTest.cpp',
            'tests/TextureLayerChromiumTest.cpp',
            'tests/ThrottledTextureUploaderTest.cpp',
            'tests/TiledLayerChromiumTest.cpp',
            'tests/TreeSynchronizerTest.cpp',
            'tests/WebAnimationTest.cpp',
            'tests/WebFloatAnimationCurveTest.cpp',
            'tests/WebLayerTest.cpp',
            'tests/WebLayerTreeViewTest.cpp',
            'tests/WebLayerTreeViewTestCommon.h',
            'tests/WebTransformAnimationCurveTest.cpp',
            'tests/WebTransformOperationsTest.cpp',
            'tests/WebTransformationMatrixTest.cpp',
        ],

        'webkit_compositor_bindings_files': [
            'src/WebAnimationCurveCommon.cpp',
            'src/WebAnimationCurveCommon.h',
            'src/WebAnimationImpl.cpp',
            'src/WebAnimationImpl.h',
            'src/WebContentLayerImpl.cpp',
            'src/WebContentLayerImpl.h',
            'src/WebExternalTextureLayerImpl.cpp',
            'src/WebExternalTextureLayerImpl.h',
            'src/WebFloatAnimationCurveImpl.cpp',
            'src/WebFloatAnimationCurveImpl.h',
            'src/WebIOSurfaceLayerImpl.cpp',
            'src/WebIOSurfaceLayerImpl.h',
            'src/WebImageLayerImpl.cpp',
            'src/WebImageLayerImpl.h',
            'src/WebLayerImpl.cpp',
            'src/WebLayerImpl.h',
            'src/WebToCCInputHandlerAdapter.cpp',
            'src/WebToCCInputHandlerAdapter.h',
            'src/WebLayerTreeViewImpl.cpp',
            'src/WebLayerTreeViewImpl.h',
            'src/WebScrollbarLayerImpl.cpp',
            'src/WebScrollbarLayerImpl.h',
            'src/WebSolidColorLayerImpl.cpp',
            'src/WebSolidColorLayerImpl.h',
            'src/WebTransformAnimationCurveImpl.cpp',
            'src/WebTransformAnimationCurveImpl.h',
            'src/WebVideoLayerImpl.cpp',
            'src/WebVideoLayerImpl.h',
        ],

        'conditions': [
            ['OS=="win"', {
                'webkit_unittest_files': [
                    'tests/LocaleWinTest.cpp',
                    # FIXME: Port PopupMenuTest to Linux and Mac.
                    'tests/PopupMenuTest.cpp',
                    'tests/TransparencyWinTest.cpp',
                    'tests/UniscribeHelperTest.cpp',
                    'tests/WebPageNewSerializerTest.cpp',
                    'tests/WebPageSerializerTest.cpp',
                ],
            }],
            ['OS=="mac"', {
                'webkit_unittest_files': [
                    'tests/LocaleMacTest.cpp',
                ],
            }],
            ['OS!="mac"', {
                'webkit_unittest_files': [
                    # Mac uses ScrollAnimatorMac instead of ScrollAnimatorNone.
                    'tests/ScrollAnimatorNoneTest.cpp',
                ],
            }],
            ['os_posix==1 and OS!="mac"', {
                'webkit_unittest_files': [
                    'tests/LocalizedDateICUTest.cpp',
                    'tests/LocalizedNumberICUTest.cpp',
                ],
            }],
            ['toolkit_uses_gtk == 1', {
                'webkit_unittest_files': [
                    'tests/WebInputEventFactoryTestGtk.cpp',
                ],
            }],
        ],
    },
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
