# Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
# Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com> 
# Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(WebCore) \
    $(WebCore)/bindings/generic \
    $(WebCore)/bindings/js \
    $(WebCore)/bindings/objc \
    $(WebCore)/css \
    $(WebCore)/dom \
    $(WebCore)/fileapi \
    $(WebCore)/html \
    $(WebCore)/html/canvas \
    $(WebCore)/inspector \
    $(WebCore)/loader/appcache \
    $(WebCore)/notifications \
    $(WebCore)/p2p \
    $(WebCore)/page \
    $(WebCore)/plugins \
    $(WebCore)/storage \
    $(WebCore)/xml \
    $(WebCore)/webaudio \
    $(WebCore)/workers \
    $(WebCore)/svg \
    $(WebCore)/testing \
    $(WebCore)/websockets \
#

DOM_CLASSES = \
    AbstractView \
    AbstractWorker \
    Attr \
    AudioBuffer \
    AudioBufferCallback \
    AudioBufferSourceNode \
    AudioChannelSplitter \
    AudioChannelMerger \
    AudioContext \
    AudioDestinationNode \
    AudioGain \
    AudioGainNode \
    AudioListener \
    AudioNode \
    AudioPannerNode \
    AudioParam \
    AudioProcessingEvent \
    AudioSourceNode \
    BiquadFilterNode \
    ConvolverNode \
    DelayNode \
    DynamicsCompressorNode \
    HighPass2FilterNode \
    JavaScriptAudioNode \
    LowPass2FilterNode \
    OfflineAudioCompletionEvent \
    RealtimeAnalyserNode \
    WaveShaperNode \
    BarInfo \
    BeforeLoadEvent \
    BeforeProcessEvent \
    Blob \
    WebKitBlobBuilder \
    CDATASection \
    CSSCharsetRule \
    CSSFontFaceRule \
    CSSImportRule \
    CSSMediaRule \
    CSSPageRule \
    CSSPrimitiveValue \
    CSSRule \
    CSSRuleList \
    CSSStyleDeclaration \
    CSSStyleRule \
    CSSStyleSheet \
    CSSUnknownRule \
    CSSValue \
    CSSValueList \
    WebGLActiveInfo \
    ArrayBufferView \
    ArrayBuffer \
    DataView \
    WebGLBuffer \
    Int8Array \
    WebGLContextAttributes \
    Float32Array \
    Float64Array \
    WebGLFramebuffer \
    CanvasGradient \
    Int32Array \
    CanvasPattern \
    WebGLProgram \
    WebGLRenderbuffer \
    CanvasRenderingContext \
    CanvasRenderingContext2D \
    WebGLRenderingContext \
    WebGLShader \
    Int16Array \
    WebGLTexture \
    WebGLUniformLocation \
    Uint8Array \
    Uint32Array \
    Uint16Array \
    CharacterData \
    ClientRect \
    ClientRectList \
    Clipboard \
    CloseEvent \
    Comment \
    CompositionEvent \
    Console \
    Coordinates \
    Counter \
    Crypto \
    CustomEvent \
    DataTransferItem \
    DataTransferItems \
    DedicatedWorkerContext \
    DOMApplicationCache \
    DOMCoreException \
    DOMFileSystem \
    DOMFileSystemSync \
    DOMFormData \
    DOMImplementation \
    DOMMimeType \
    DOMMimeTypeArray \
    DOMParser \
    DOMPlugin \
    DOMPluginArray \
    DOMSelection \
    DOMStringList \
    DOMStringMap \
    DOMSettableTokenList \
    DOMTokenList \
    DOMURL \
    DOMWindow \
    DataTransferItem \
    DataTransferItems \
    Database \
    DatabaseCallback \
    DatabaseSync \
    DeviceMotionEvent \
    DeviceOrientationEvent \
    DirectoryEntry \
    DirectoryEntrySync \
    DirectoryReader \
    DirectoryReaderSync \
    Document \
    DocumentFragment \
    DocumentType \
    Element \
    ElementTimeControl \
    Entity \
    EntityReference \
    EntriesCallback \
    Entry \
    EntrySync \
    EntryArray \
    EntryArraySync \
    EntryCallback \
    ErrorCallback \
    ErrorEvent \
    Event \
    EventException \
    EventListener \
    EventSource \
    EventTarget \
    File \
    FileCallback \
    FileEntry \
    FileEntrySync \
    FileException \
    FileError \
    FileList \
    FileReader \
    FileReaderSync \
    FileWriter \
    FileWriterCallback \
    FileWriterSync \
    FileSystemCallback \
    WebKitFlags \
    Geolocation \
    Geoposition \
    HashChangeEvent \
    History \
    HTMLAllCollection \
    HTMLAnchorElement \
    HTMLAppletElement \
    HTMLAreaElement \
    HTMLAudioElement \
    HTMLBRElement \
    HTMLBaseElement \
    HTMLBaseFontElement \
    HTMLBlockquoteElement \
    HTMLBodyElement \
    HTMLButtonElement \
    HTMLCanvasElement \
    HTMLCollection \
    HTMLDataListElement \
    HTMLDetailsElement \
    HTMLDListElement \
    HTMLDirectoryElement \
    HTMLDivElement \
    HTMLDocument \
    HTMLElement \
    HTMLEmbedElement \
    HTMLFieldSetElement \
    HTMLFontElement \
    HTMLFormElement \
    HTMLFrameElement \
    HTMLFrameSetElement \
    HTMLHRElement \
    HTMLHeadElement \
    HTMLHeadingElement \
    HTMLHtmlElement \
    HTMLIFrameElement \
    HTMLImageElement \
    HTMLInputElement \
    HTMLIsIndexElement \
    HTMLKeygenElement \
    HTMLLIElement \
    HTMLLabelElement \
    HTMLLegendElement \
    HTMLLinkElement \
    HTMLMapElement \
    HTMLMarqueeElement \
    HTMLMediaElement \
    HTMLMenuElement \
    HTMLMetaElement \
    HTMLMeterElement \
    HTMLModElement \
    HTMLOListElement \
    HTMLObjectElement \
    HTMLOptGroupElement \
    HTMLOptionElement \
    HTMLOptionsCollection \
    HTMLOutputElement \
    HTMLParagraphElement \
    HTMLParamElement \
    HTMLPreElement \
    HTMLProgressElement \
    HTMLQuoteElement \
    HTMLScriptElement \
    HTMLSelectElement \
    HTMLSourceElement \
    HTMLStyleElement \
    HTMLTableCaptionElement \
    HTMLTableCellElement \
    HTMLTableColElement \
    HTMLTableElement \
    HTMLTableRowElement \
    HTMLTableSectionElement \
    HTMLTextAreaElement \
    HTMLTitleElement \
    HTMLTrackElement \
    HTMLUListElement \
    HTMLUnknownElement \
    HTMLVideoElement \
    IDBAny \
    IDBCursor \
    IDBDatabaseError \
    IDBDatabaseException \
    IDBDatabase \
    IDBFactory \
    IDBIndex \
    IDBKey \
    IDBKeyRange \
    IDBObjectStore \
    IDBRequest \
    IDBTransaction \
    ImageData \
    InjectedScriptHost \
    InspectorFrontendHost \
    Internals \
    KeyboardEvent \
    LocalMediaStream \
    Location \
    MediaError \
    MediaList \
    MediaQueryList \
    MediaQueryListListener \
    MediaStream \
    MediaStreamEvent \
    MediaStreamList \
    MediaStreamTrack \
    MediaStreamTrackList \
    MemoryInfo \
    MessageChannel \
    MessageEvent \
    MessagePort \
    Metadata \
    MetadataCallback \
    MouseEvent \
    MutationEvent \
    NamedNodeMap \
    Navigator \
    NavigatorUserMediaError \
    NavigatorUserMediaErrorCallback \
    NavigatorUserMediaSuccessCallback \
    Node \
    NodeFilter \
    NodeIterator \
    NodeList \
    Notation \
    Notification \
    NotificationCenter \
    OESStandardDerivatives \
    OESTextureFloat \
     OESVertexArrayObject \
     WebGLVertexArrayObjectOES \
    OperationNotAllowedException \
    OverflowEvent \
    PageTransitionEvent \
    PeerConnection \
    Performance \
    PerformanceNavigation \
    PerformanceTiming \
    PopStateEvent \
    PositionCallback \
    PositionError \
    PositionErrorCallback \
    ProcessingInstruction \
    ProgressEvent \
    RGBColor \
    Range \
    RangeException \
    Rect \
    SharedWorker \
    SharedWorkerContext \
    ScriptProfile \
    ScriptProfileNode \
    SignalingCallback \
    SpeechInputEvent \
    SpeechInputResult \
    SpeechInputResultList \
    SQLError \
    SQLException \
    SQLResultSet \
    SQLResultSetRowList \
    SQLStatementCallback \
    SQLStatementErrorCallback \
    SQLTransaction \
    SQLTransactionCallback \
    SQLTransactionErrorCallback \
    SQLTransactionSync \
    SQLTransactionSyncCallback \
    Storage \
    StorageEvent \
    StorageInfo \
    StorageInfoErrorCallback \
    StorageInfoQuotaCallback \
    StorageInfoUsageCallback \
    StringCallback \
    SVGAElement \
    SVGAltGlyphDefElement \
    SVGAltGlyphElement \
    SVGAltGlyphItemElement \
    SVGAngle \
    SVGAnimateColorElement \
    SVGAnimateElement \
    SVGAnimateMotionElement \
    SVGAnimateTransformElement \
    SVGAnimatedAngle \
    SVGAnimatedBoolean \
    SVGAnimatedEnumeration \
    SVGAnimatedInteger \
    SVGAnimatedLength \
    SVGAnimatedLengthList \
    SVGAnimatedNumber \
    SVGAnimatedNumberList \
    SVGAnimatedPreserveAspectRatio \
    SVGAnimatedRect \
    SVGAnimatedString \
    SVGAnimatedTransformList \
    SVGAnimationElement \
    SVGCircleElement \
    SVGClipPathElement \
    SVGColor \
    SVGComponentTransferFunctionElement \
    SVGCursorElement \
    SVGDefsElement \
    SVGDescElement \
    SVGDocument \
    SVGElement \
    SVGElementInstance \
    SVGElementInstanceList \
    SVGEllipseElement \
    SVGException \
    SVGExternalResourcesRequired \
    SVGFEBlendElement \
    SVGFEColorMatrixElement \
    SVGFEComponentTransferElement \
    SVGFECompositeElement \
    SVGFEConvolveMatrixElement \
    SVGFEDiffuseLightingElement \
    SVGFEDisplacementMapElement \
    SVGFEDistantLightElement \
    SVGFEDropShadowElement \
    SVGFEFloodElement \
    SVGFEFuncAElement \
    SVGFEFuncBElement \
    SVGFEFuncGElement \
    SVGFEFuncRElement \
    SVGFEGaussianBlurElement \
    SVGFEImageElement \
    SVGFEMergeElement \
    SVGFEMergeNodeElement \
    SVGFEMorphologyElement \
    SVGFEOffsetElement \
    SVGFEPointLightElement \
    SVGFESpecularLightingElement \
    SVGFESpotLightElement \
    SVGFETileElement \
    SVGFETurbulenceElement \
    SVGFilterElement \
    SVGFilterPrimitiveStandardAttributes \
    SVGFitToViewBox \
    SVGFontElement \
    SVGFontFaceElement \
    SVGFontFaceFormatElement \
    SVGFontFaceNameElement \
    SVGFontFaceSrcElement \
    SVGFontFaceUriElement \
    SVGForeignObjectElement \
    SVGGElement \
    SVGGlyphElement \
    SVGGlyphRefElement \
    SVGGradientElement \
    SVGHKernElement \
    SVGImageElement \
    SVGLangSpace \
    SVGLength \
    SVGLengthList \
    SVGLineElement \
    SVGLinearGradientElement \
    SVGLocatable \
    SVGMarkerElement \
    SVGMaskElement \
    SVGMatrix \
    SVGMetadataElement \
    SVGMissingGlyphElement \
    SVGMPathElement \
    SVGNumber \
    SVGNumberList \
    SVGPaint \
    SVGPathElement \
    SVGPathSeg \
    SVGPathSegArcAbs \
    SVGPathSegArcRel \
    SVGPathSegClosePath \
    SVGPathSegCurvetoCubicAbs \
    SVGPathSegCurvetoCubicRel \
    SVGPathSegCurvetoCubicSmoothAbs \
    SVGPathSegCurvetoCubicSmoothRel \
    SVGPathSegCurvetoQuadraticAbs \
    SVGPathSegCurvetoQuadraticRel \
    SVGPathSegCurvetoQuadraticSmoothAbs \
    SVGPathSegCurvetoQuadraticSmoothRel \
    SVGPathSegLinetoAbs \
    SVGPathSegLinetoHorizontalAbs \
    SVGPathSegLinetoHorizontalRel \
    SVGPathSegLinetoRel \
    SVGPathSegLinetoVerticalAbs \
    SVGPathSegLinetoVerticalRel \
    SVGPathSegList \
    SVGPathSegMovetoAbs \
    SVGPathSegMovetoRel \
    SVGPatternElement \
    SVGPoint \
    SVGPointList \
    SVGPolygonElement \
    SVGPolylineElement \
    SVGPreserveAspectRatio \
    SVGRadialGradientElement \
    SVGRect \
    SVGRectElement \
    SVGRenderingIntent \
    SVGSVGElement \
    SVGScriptElement \
    SVGSetElement \
    SVGStopElement \
    SVGStringList \
    SVGStylable \
    SVGStyleElement \
    SVGSwitchElement \
    SVGSymbolElement \
    SVGTRefElement \
    SVGTSpanElement \
    SVGTests \
    SVGTextContentElement \
    SVGTextElement \
    SVGTextPathElement \
    SVGTextPositioningElement \
    SVGTitleElement \
    SVGTransform \
    SVGTransformList \
    SVGTransformable \
    SVGURIReference \
    SVGUnitTypes \
    SVGUseElement \
    SVGViewElement \
    SVGVKernElement \
    SVGZoomAndPan \
    SVGZoomEvent \
    Screen \
    StringCallback \
    StyleMedia \
    StyleSheet \
    StyleSheetList \
    Text \
    TextEvent \
    TextMetrics \
    TimeRanges \
    Touch \
    TouchEvent \
    TouchList \
    TreeWalker \
    UIEvent \
    ValidityState \
    WebKitAnimation \
    WebKitAnimationEvent \
    WebKitAnimationList \
    WebKitCSSKeyframeRule \
    WebKitCSSKeyframesRule \
    WebKitCSSMatrix \
    WebKitCSSTransformValue \
    WebKitLoseContext \
    WebKitPoint \
    WebKitTransitionEvent \
    WebSocket \
    WheelEvent \
    Worker \
    WorkerContext \
    WorkerLocation \
    WorkerNavigator \
    XMLHttpRequest \
    XMLHttpRequestException \
    XMLHttpRequestProgressEvent \
    XMLHttpRequestUpload \
    XMLSerializer \
    XPathEvaluator \
    XPathException \
    XPathExpression \
    XPathNSResolver \
    XPathResult \
    XSLTProcessor \
#

.PHONY : all

JS_DOM_HEADERS=$(filter-out JSMediaQueryListListener.h JSEventListener.h JSEventTarget.h,$(DOM_CLASSES:%=JS%.h))

WEB_DOM_HEADERS :=
ifeq ($(findstring BUILDING_WX,$(FEATURE_DEFINES)), BUILDING_WX)
WEB_DOM_HEADERS := $(filter-out WebDOMXSLTProcessor.h WebDOMEventTarget.h,$(DOM_CLASSES:%=WebDOM%.h))
endif # BUILDING_WX

all : \
    $(JS_DOM_HEADERS) \
    $(WEB_DOM_HEADERS) \
    \
    JSJavaScriptCallFrame.h \
    \
    CSSGrammar.cpp \
    CSSPropertyNames.h \
    CSSValueKeywords.h \
    ColorData.cpp \
    DocTypeStrings.cpp \
    HTMLElementFactory.cpp \
    HTMLEntityTable.cpp \
    HTMLNames.cpp \
    JSSVGElementWrapperFactory.cpp \
    SVGElementFactory.cpp \
    SVGNames.cpp \
    UserAgentStyleSheets.h \
    XLinkNames.cpp \
    XMLNSNames.cpp \
    XMLNames.cpp \
    MathMLElementFactory.cpp \
    MathMLNames.cpp \
    XPathGrammar.cpp \
    tokenizer.cpp \
#

# --------

ADDITIONAL_IDL_DEFINES :=

ifeq ($(OS),MACOS)

FRAMEWORK_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(FRAMEWORK_SEARCH_PATHS) | perl -e 'print "-F " . join(" -F ", split(" ", <>));')

ifeq ($(shell gcc -E -P -dM $(FRAMEWORK_FLAGS) WebCore/ForwardingHeaders/wtf/Platform.h | grep ENABLE_DASHBOARD_SUPPORT | cut -d' ' -f3), 1)
    ENABLE_DASHBOARD_SUPPORT = 1
else
    ENABLE_DASHBOARD_SUPPORT = 0
endif

ifeq ($(shell gcc -E -P -dM $(FRAMEWORK_FLAGS) WebCore/ForwardingHeaders/wtf/Platform.h | grep ENABLE_ORIENTATION_EVENTS | cut -d' ' -f3), 1)
    ENABLE_ORIENTATION_EVENTS = 1
else
    ENABLE_ORIENTATION_EVENTS = 0
endif

else

ifndef ENABLE_DASHBOARD_SUPPORT
    ENABLE_DASHBOARD_SUPPORT = 0
endif

ifndef ENABLE_ORIENTATION_EVENTS
    ENABLE_ORIENTATION_EVENTS = 0
endif

endif # MACOS

ifeq ($(ENABLE_ORIENTATION_EVENTS), 1)
    ADDITIONAL_IDL_DEFINES := $(ADDITIONAL_IDL_DEFINES) ENABLE_ORIENTATION_EVENTS
endif

# --------

# CSS property names and value keywords

WEBCORE_CSS_PROPERTY_NAMES := $(WebCore)/css/CSSPropertyNames.in
WEBCORE_CSS_VALUE_KEYWORDS := $(WebCore)/css/CSSValueKeywords.in

ifeq ($(findstring ENABLE_SVG,$(FEATURE_DEFINES)), ENABLE_SVG)
    WEBCORE_CSS_PROPERTY_NAMES := $(WEBCORE_CSS_PROPERTY_NAMES) $(WebCore)/css/SVGCSSPropertyNames.in
    WEBCORE_CSS_VALUE_KEYWORDS := $(WEBCORE_CSS_VALUE_KEYWORDS) $(WebCore)/css/SVGCSSValueKeywords.in
endif

ifeq ($(ENABLE_DASHBOARD_SUPPORT), 1)
    WEBCORE_CSS_PROPERTY_NAMES := $(WEBCORE_CSS_PROPERTY_NAMES) $(WebCore)/css/DashboardSupportCSSPropertyNames.in
endif

CSSPropertyNames.h : $(WEBCORE_CSS_PROPERTY_NAMES) css/makeprop.pl
	cat $(WEBCORE_CSS_PROPERTY_NAMES) > CSSPropertyNames.in
	perl -I$(WebCore)/bindings/scripts "$(WebCore)/css/makeprop.pl" --defines "$(FEATURE_DEFINES)"

CSSValueKeywords.h : $(WEBCORE_CSS_VALUE_KEYWORDS) css/makevalues.pl
	cat $(WEBCORE_CSS_VALUE_KEYWORDS) > CSSValueKeywords.in
	perl -I$(WebCore)/bindings/scripts "$(WebCore)/css/makevalues.pl" --defines "$(FEATURE_DEFINES)"

# --------

# DOCTYPE strings

DocTypeStrings.cpp : html/DocTypeStrings.gperf $(WebCore)/make-hash-tools.pl
	perl $(WebCore)/make-hash-tools.pl . $(WebCore)/html/DocTypeStrings.gperf

# --------

# XMLViewer CSS

all : XMLViewerCSS.h

XMLViewerCSS.h : xml/XMLViewer.css
	perl $(WebCore)/inspector/xxd.pl XMLViewer_css $(WebCore)/xml/XMLViewer.css XMLViewerCSS.h

# --------

# XMLViewer JS

all : XMLViewerJS.h

XMLViewerJS.h : xml/XMLViewer.js
	perl $(WebCore)/inspector/xxd.pl XMLViewer_js $(WebCore)/xml/XMLViewer.js XMLViewerJS.h

# --------

# HTML entity names

HTMLEntityTable.cpp : html/parser/HTMLEntityNames.in $(WebCore)/html/parser/create-html-entity-table
	python $(WebCore)/html/parser/create-html-entity-table -o HTMLEntityTable.cpp $(WebCore)/html/parser/HTMLEntityNames.in

# --------

# color names

ColorData.cpp : platform/ColorData.gperf $(WebCore)/make-hash-tools.pl
	perl $(WebCore)/make-hash-tools.pl . $(WebCore)/platform/ColorData.gperf

# --------

# CSS tokenizer

tokenizer.cpp : css/tokenizer.flex css/maketokenizer
	flex -t $< | perl $(WebCore)/css/maketokenizer > $@

# --------

# CSS grammar
# NOTE: Older versions of bison do not inject an inclusion guard, so we add one.

CSSGrammar.cpp : css/CSSGrammar.y
	bison -d -p cssyy $< -o $@
	touch CSSGrammar.cpp.h
	touch CSSGrammar.hpp
	echo '#ifndef CSSGrammar_h' > CSSGrammar.h
	echo '#define CSSGrammar_h' >> CSSGrammar.h
	cat CSSGrammar.cpp.h CSSGrammar.hpp >> CSSGrammar.h
	echo '#endif' >> CSSGrammar.h
	rm -f CSSGrammar.cpp.h CSSGrammar.hpp

# --------

# XPath grammar
# NOTE: Older versions of bison do not inject an inclusion guard, so we add one.

XPathGrammar.cpp : xml/XPathGrammar.y $(PROJECT_FILE)
	bison -d -p xpathyy $< -o $@
	touch XPathGrammar.cpp.h
	touch XPathGrammar.hpp
	echo '#ifndef XPathGrammar_h' > XPathGrammar.h
	echo '#define XPathGrammar_h' >> XPathGrammar.h
	cat XPathGrammar.cpp.h XPathGrammar.hpp >> XPathGrammar.h
	echo '#endif' >> XPathGrammar.h
	rm -f XPathGrammar.cpp.h XPathGrammar.hpp

# --------

# user agent style sheets

USER_AGENT_STYLE_SHEETS = $(WebCore)/css/html.css $(WebCore)/css/quirks.css $(WebCore)/css/view-source.css $(WebCore)/css/themeWin.css $(WebCore)/css/themeWinQuirks.css 

ifeq ($(findstring ENABLE_SVG,$(FEATURE_DEFINES)), ENABLE_SVG)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/svg.css 
endif

ifeq ($(findstring ENABLE_MATHML,$(FEATURE_DEFINES)), ENABLE_MATHML)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mathml.css
endif

ifeq ($(findstring ENABLE_VIDEO,$(FEATURE_DEFINES)), ENABLE_VIDEO)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mediaControls.css
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/mediaControlsQuickTime.css
endif

ifeq ($(findstring ENABLE_FULLSCREEN_API,$(FEATURE_DEFINES)), ENABLE_FULLSCREEN_API)
    USER_AGENT_STYLE_SHEETS := $(USER_AGENT_STYLE_SHEETS) $(WebCore)/css/fullscreen.css $(WebCore)/css/fullscreenQuickTime.css
endif

UserAgentStyleSheets.h : css/make-css-file-arrays.pl bindings/scripts/preprocessor.pm $(USER_AGENT_STYLE_SHEETS)
	perl -I$(WebCore)/bindings/scripts $< --defines "$(FEATURE_DEFINES)" $@ UserAgentStyleSheetsData.cpp $(USER_AGENT_STYLE_SHEETS)

# --------

# HTML tag and attribute names

ifeq ($(findstring ENABLE_DATALIST,$(FEATURE_DEFINES)), ENABLE_DATALIST)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_DATALIST=1
endif

ifeq ($(findstring ENABLE_DETAILS,$(FEATURE_DEFINES)), ENABLE_DETAILS)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_DETAILS=1
endif

ifeq ($(findstring ENABLE_METER_TAG,$(FEATURE_DEFINES)), ENABLE_METER_TAG)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_METER_TAG=1
endif

ifeq ($(findstring ENABLE_PROGRESS_TAG,$(FEATURE_DEFINES)), ENABLE_PROGRESS_TAG)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_PROGRESS_TAG=1
endif

ifeq ($(findstring ENABLE_VIDEO,$(FEATURE_DEFINES)), ENABLE_VIDEO)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_VIDEO=1
endif

ifeq ($(findstring ENABLE_VIDEO_TRACK,$(FEATURE_DEFINES)), ENABLE_VIDEO_TRACK)
    HTML_FLAGS := $(HTML_FLAGS) ENABLE_VIDEO_TRACK=0
endif

ifdef HTML_FLAGS

HTMLElementFactory.cpp HTMLNames.cpp : dom/make_names.pl html/HTMLTagNames.in html/HTMLAttributeNames.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/html/HTMLTagNames.in --attrs $(WebCore)/html/HTMLAttributeNames.in --factory --wrapperFactory --extraDefines "$(HTML_FLAGS)"

else

HTMLElementFactory.cpp HTMLNames.cpp : dom/make_names.pl html/HTMLTagNames.in html/HTMLAttributeNames.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/html/HTMLTagNames.in --attrs $(WebCore)/html/HTMLAttributeNames.in --factory --wrapperFactory

endif

JSHTMLElementWrapperFactory.cpp : HTMLNames.cpp

XMLNSNames.cpp : dom/make_names.pl xml/xmlnsattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/xml/xmlnsattrs.in

XMLNames.cpp : dom/make_names.pl xml/xmlattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/xml/xmlattrs.in

# --------

# SVG tag and attribute names, and element factory

ifeq ($(findstring ENABLE_SVG_USE,$(FEATURE_DEFINES)), ENABLE_SVG_USE)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_USE=1
endif

ifeq ($(findstring ENABLE_SVG_FONTS,$(FEATURE_DEFINES)), ENABLE_SVG_FONTS)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_FONTS=1
endif

ifeq ($(findstring ENABLE_FILTERS,$(FEATURE_DEFINES)), ENABLE_FILTERS)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_FILTERS=1
endif

ifeq ($(findstring ENABLE_SVG_AS_IMAGE,$(FEATURE_DEFINES)), ENABLE_SVG_AS_IMAGE)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_AS_IMAGE=1
endif

ifeq ($(findstring ENABLE_SVG_ANIMATION,$(FEATURE_DEFINES)), ENABLE_SVG_ANIMATION)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_ANIMATION=1
endif

ifeq ($(findstring ENABLE_SVG_FOREIGN_OBJECT,$(FEATURE_DEFINES)), ENABLE_SVG_FOREIGN_OBJECT)
    SVG_FLAGS := $(SVG_FLAGS) ENABLE_SVG_FOREIGN_OBJECT=1
endif

# SVG tag and attribute names (need to pass an extra flag if svg experimental features are enabled)

ifdef SVG_FLAGS

SVGElementFactory.cpp SVGNames.cpp : dom/make_names.pl svg/svgtags.in svg/svgattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/svg/svgtags.in --attrs $(WebCore)/svg/svgattrs.in --extraDefines "$(SVG_FLAGS)" --factory --wrapperFactory
else

SVGElementFactory.cpp SVGNames.cpp : dom/make_names.pl svg/svgtags.in svg/svgattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/svg/svgtags.in --attrs $(WebCore)/svg/svgattrs.in --factory --wrapperFactory

endif

JSSVGElementWrapperFactory.cpp : SVGNames.cpp

XLinkNames.cpp : dom/make_names.pl svg/xlinkattrs.in
	perl -I $(WebCore)/bindings/scripts $< --attrs $(WebCore)/svg/xlinkattrs.in

# --------
 
# MathML tag and attribute names, and element factory

MathMLElementFactory.cpp MathMLNames.cpp : dom/make_names.pl mathml/mathtags.in mathml/mathattrs.in
	perl -I $(WebCore)/bindings/scripts $< --tags $(WebCore)/mathml/mathtags.in --attrs $(WebCore)/mathml/mathattrs.in --factory --wrapperFactory

# --------

# Common generator things

GENERATE_SCRIPTS = \
    bindings/scripts/CodeGenerator.pm \
    bindings/scripts/IDLParser.pm \
    bindings/scripts/IDLStructure.pm \
    bindings/scripts/generate-bindings.pl \
    bindings/scripts/preprocessor.pm

generator_script = perl $(addprefix -I $(WebCore)/, $(sort $(dir $(1)))) $(WebCore)/bindings/scripts/generate-bindings.pl

# JS bindings generator

IDL_INCLUDES = \
    $(WebCore)/dom \
    $(WebCore)/fileapi \
    $(WebCore)/html \
    $(WebCore)/css \
    $(WebCore)/p2p \
    $(WebCore)/page \
    $(WebCore)/notifications \
    $(WebCore)/xml \
    $(WebCore)/svg

IDL_COMMON_ARGS = $(IDL_INCLUDES:%=--include %) --write-dependencies --outputDir .

JS_BINDINGS_SCRIPTS = $(GENERATE_SCRIPTS) bindings/scripts/CodeGeneratorJS.pm

JS%.h : %.idl $(JS_BINDINGS_SCRIPTS)
	$(call generator_script, $(JS_BINDINGS_SCRIPTS)) $(IDL_COMMON_ARGS) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_JAVASCRIPT" --generator JS $<

# Inspector interfaces generator

Inspector.idl : Inspector.json inspector/generate-inspector-idl
	python $(WebCore)/inspector/generate-inspector-idl -o Inspector.idl $(WebCore)/inspector/Inspector.json

all : InspectorFrontend.h

INSPECTOR_GENERATOR_SCRIPTS = $(GENERATE_SCRIPTS) inspector/CodeGeneratorInspector.pm

InspectorFrontend.h : Inspector.idl $(INSPECTOR_GENERATOR_SCRIPTS)
	$(call generator_script, $(INSPECTOR_GENERATOR_SCRIPTS)) --outputDir . --defines "$(FEATURE_DEFINES) LANGUAGE_JAVASCRIPT" --generator Inspector $<

all : InjectedScriptSource.h

InjectedScriptSource.h : InjectedScriptSource.js
	perl $(WebCore)/inspector/xxd.pl InjectedScriptSource_js $(WebCore)/inspector/InjectedScriptSource.js InjectedScriptSource.h

-include $(JS_DOM_HEADERS:.h=.dep)

ifeq ($(findstring BUILDING_WX,$(FEATURE_DEFINES)), BUILDING_WX)
CPP_BINDINGS_SCRIPTS = $(GENERATE_SCRIPTS) bindings/scripts/CodeGeneratorCPP.pm

WebDOM%.h : %.idl $(CPP_BINDINGS_SCRIPTS)
	$(call generator_script, $(CPP_BINDINGS_SCRIPTS)) $(IDL_COMMON_ARGS) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_CPP" --generator CPP $<
endif # BUILDING_WX

# ------------------------

# Mac-specific rules

ifeq ($(OS),MACOS)

OBJC_DOM_HEADERS=$(filter-out DOMDOMWindow.h DOMDOMMimeType.h DOMDOMPlugin.h,$(DOM_CLASSES:%=DOM%.h))

all : $(OBJC_DOM_HEADERS)

all : CharsetData.cpp

# --------

# character set name table

CharsetData.cpp : platform/text/mac/make-charset-table.pl platform/text/mac/character-sets.txt platform/text/mac/mac-encodings.txt
	perl $^ kTextEncoding > $@

# --------

ifneq ($(ACTION),installhdrs)

all : WebCore.exp WebCore.LP64.exp

WebCore.exp : $(BUILT_PRODUCTS_DIR)/WebCoreExportFileGenerator
	$^ > $@

# Switch NSRect, NSSize and NSPoint with their CG counterparts for the 64-bit exports file.
WebCore.LP64.exp : WebCore.exp
	cat $^ | sed -e s/7_NSRect/6CGRect/ -e s/7_NSSize/6CGSize/ -e s/8_NSPoint/7CGPoint/ > $@

endif # installhdrs

# --------

# Objective-C bindings

DOM_BINDINGS_SCRIPTS = $(GENERATE_BINDING_SCRIPTS) bindings/scripts/CodeGeneratorObjC.pm
DOM%.h : %.idl $(DOM_BINDINGS_SCRIPTS) bindings/objc/PublicDOMInterfaces.h
	$(call generator_script, $(DOM_BINDINGS_SCRIPTS)) $(IDL_COMMON_ARGS) --defines "$(FEATURE_DEFINES) $(ADDITIONAL_IDL_DEFINES) LANGUAGE_OBJECTIVE_C" --generator ObjC $<

-include $(OBJC_DOM_HEADERS:.h=.dep)

# --------

endif # MACOS

# ------------------------

# Header detection

ifeq ($(OS),Windows_NT)

all : WebCoreHeaderDetection.h

WebCoreHeaderDetection.h : DerivedSources.make
	if [ -f "$(WEBKITLIBRARIESDIR)/include/AVFoundationCF/AVCFBase.h" ]; then echo "#define HAVE_AVCF 1" > $@; else echo > $@; fi

endif # Windows_NT
