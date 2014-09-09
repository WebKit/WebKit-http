/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebDataSource.h"

#import "WebArchive.h"
#import "WebArchiveInternal.h"
#import "WebDataSourceInternal.h"
#import "WebDocument.h"
#import "WebDocumentLoaderMac.h"
#import "WebFrameInternal.h"
#import "WebFrameLoadDelegate.h"
#import "WebFrameLoaderClient.h"
#import "WebFrameViewInternal.h"
#import "WebHTMLRepresentation.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitLogging.h"
#import "WebKitStatisticsPrivate.h"
#import "WebKitNSStringExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebPDFRepresentation.h"
#import "WebResourceInternal.h"
#import "WebResourceLoadDelegate.h"
#import "WebViewInternal.h"
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/URL.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/ResourceBuffer.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreURLResponse.h>
#import <WebKitLegacy/DOMHTML.h>
#import <WebKitLegacy/DOMPrivate.h>
#import <runtime/InitializeThreading.h>
#import <wtf/Assertions.h>
#import <wtf/MainThread.h>
#import <wtf/RefPtr.h>
#import <wtf/RetainPtr.h>
#import <wtf/RunLoop.h>

#if PLATFORM(IOS)
#import "WebPDFViewIOS.h"
#endif

#if USE(QUICK_LOOK)
#import <WebCore/QuickLook.h>
#endif

using namespace WebCore;

class WebDataSourcePrivate
{
public:
    WebDataSourcePrivate(PassRefPtr<WebDocumentLoaderMac> loader)
        : loader(loader)
        , representationFinishedLoading(NO)
        , includedInWebKitStatistics(NO)
#if PLATFORM(IOS)
        , _dataSourceDelegate(nil)
#endif
    {
        ASSERT(this->loader);
    }
    ~WebDataSourcePrivate()
    {
        if (loader) {
            ASSERT(!loader->isLoading());
            loader->detachDataSource();
        }
    }

    RefPtr<WebDocumentLoaderMac> loader;
    RetainPtr<id<WebDocumentRepresentation> > representation;
    BOOL representationFinishedLoading;
    BOOL includedInWebKitStatistics;
#if PLATFORM(IOS)
    NSObject<WebDataSourcePrivateDelegate> *_dataSourceDelegate;
#endif
};

static inline WebDataSourcePrivate* toPrivate(void* privateAttribute)
{
    return reinterpret_cast<WebDataSourcePrivate*>(privateAttribute);
}

#if ENABLE(DISK_IMAGE_CACHE) && PLATFORM(IOS)
static void BufferMemoryMapped(PassRefPtr<SharedBuffer> buffer, SharedBuffer::CompletionStatus mapStatus, SharedBuffer::MemoryMappedNotifyCallbackData data)
{
    NSObject<WebDataSourcePrivateDelegate> *delegate = [(WebDataSource *)data dataSourceDelegate];
    if (mapStatus == SharedBuffer::Succeeded)
        [delegate dataSourceMemoryMapped];
    else
        [delegate dataSourceMemoryMapFailed];
}
#endif

@interface WebDataSource (WebFileInternal)
@end

@implementation WebDataSource (WebFileInternal)

- (void)_setRepresentation:(id<WebDocumentRepresentation>)representation
{
    toPrivate(_private)->representation = representation;
    toPrivate(_private)->representationFinishedLoading = NO;
}

static inline void addTypesFromClass(NSMutableDictionary *allTypes, Class objCClass, NSArray *supportTypes)
{
    NSEnumerator *enumerator = [supportTypes objectEnumerator];
    ASSERT(enumerator != nil);
    NSString *mime = nil;
    while ((mime = [enumerator nextObject]) != nil) {
        // Don't clobber previously-registered classes.
        if ([allTypes objectForKey:mime] == nil)
            [allTypes setObject:objCClass forKey:mime];
    }
}

+ (Class)_representationClassForMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins
{
    Class repClass;
    return [WebView _viewClass:nil andRepresentationClass:&repClass forMIMEType:MIMEType allowingPlugins:allowPlugins] ? repClass : nil;
}
@end

@implementation WebDataSource (WebPrivate)

+ (void)initialize
{
    if (self == [WebDataSource class]) {
#if !PLATFORM(IOS)
        JSC::initializeThreading();
        WTF::initializeMainThreadToProcessMainThread();
        RunLoop::initializeMainRunLoop();
#endif
        WebCoreObjCFinalizeOnMainThread(self);
    }
}

- (NSError *)_mainDocumentError
{
    return toPrivate(_private)->loader->mainDocumentError();
}

- (void)_addSubframeArchives:(NSArray *)subframeArchives
{
    // FIXME: This SPI is poor, poor design.  Can we come up with another solution for those who need it?
    NSEnumerator *enumerator = [subframeArchives objectEnumerator];
    WebArchive *archive;
    while ((archive = [enumerator nextObject]) != nil)
        toPrivate(_private)->loader->addAllArchiveResources([archive _coreLegacyWebArchive]);
}

#if !PLATFORM(IOS)
- (NSFileWrapper *)_fileWrapperForURL:(NSURL *)URL
{
    if ([URL isFileURL])
        return [[[NSFileWrapper alloc] initWithURL:[URL URLByResolvingSymlinksInPath] options:0 error:nullptr] autorelease];

    WebResource *resource = [self subresourceForURL:URL];
    if (resource)
        return [resource _fileWrapperRepresentation];
    
    NSCachedURLResponse *cachedResponse = [[self _webView] _cachedResponseForURL:URL];
    if (cachedResponse) {
        NSFileWrapper *wrapper = [[[NSFileWrapper alloc] initRegularFileWithContents:[cachedResponse data]] autorelease];
        [wrapper setPreferredFilename:[[cachedResponse response] suggestedFilename]];
        return wrapper;
    }
    
    return nil;
}
#endif

- (NSString *)_responseMIMEType
{
    return [[self response] MIMEType];
}

- (BOOL)_transferApplicationCache:(NSString*)destinationBundleIdentifier
{
    if (!toPrivate(_private)->loader)
        return NO;
        
    NSString *cacheDir = [NSString _webkit_localCacheDirectoryWithBundleIdentifier:destinationBundleIdentifier];
    
    return ApplicationCacheStorage::storeCopyOfCache(cacheDir, toPrivate(_private)->loader->applicationCacheHost());
}

- (void)_setDeferMainResourceDataLoad:(BOOL)flag
{
    if (!toPrivate(_private)->loader)
        return;

    toPrivate(_private)->loader->setDeferMainResourceDataLoad(flag);
}

#if PLATFORM(IOS)
- (void)_setOverrideTextEncodingName:(NSString *)encoding
{
    toPrivate(_private)->loader->setOverrideEncoding([encoding UTF8String]);
}
#endif

- (void)_setAllowToBeMemoryMapped
{
#if ENABLE(DISK_IMAGE_CACHE) && PLATFORM(IOS)
    RefPtr<ResourceBuffer> mainResourceBuffer = toPrivate(_private)->loader->mainResourceData();
    if (!mainResourceBuffer)
        return;

    RefPtr<SharedBuffer> mainResourceData = mainResourceBuffer->sharedBuffer();
    if (!mainResourceData)
        return;

    if (mainResourceData->memoryMappedNotificationCallback() != BufferMemoryMapped) {
        ASSERT(!mainResourceData->memoryMappedNotificationCallback() && !mainResourceData->memoryMappedNotificationCallbackData());
        mainResourceData->setMemoryMappedNotificationCallback(BufferMemoryMapped, self);
    }

    switch (mainResourceData->allowToBeMemoryMapped()) {
    case SharedBuffer::SuccessAlreadyMapped:
        [[self dataSourceDelegate] dataSourceMemoryMapped];
        return;
    case SharedBuffer::PreviouslyQueuedForMapping:
    case SharedBuffer::QueuedForMapping:
        return;
    case SharedBuffer::FailureCacheFull:
        [[self dataSourceDelegate] dataSourceMemoryMapFailed];
        return;
    }
    ASSERT_NOT_REACHED();
#endif
}

- (void)setDataSourceDelegate:(NSObject<WebDataSourcePrivateDelegate> *)delegate
{
#if ENABLE(DISK_IMAGE_CACHE) && PLATFORM(IOS)
    ASSERT(!toPrivate(_private)->_dataSourceDelegate);
    toPrivate(_private)->_dataSourceDelegate = delegate;
#endif
}

- (NSObject<WebDataSourcePrivateDelegate> *)dataSourceDelegate
{
#if ENABLE(DISK_IMAGE_CACHE) && PLATFORM(IOS)
    return toPrivate(_private)->_dataSourceDelegate;
#else
    return nullptr;
#endif
}

@end

@implementation WebDataSource (WebInternal)

- (void)_finishedLoading
{
    toPrivate(_private)->representationFinishedLoading = YES;
    [[self representation] finishedLoadingWithDataSource:self];
}

- (void)_receivedData:(NSData *)data
{
    // protect self temporarily, as the bridge receivedData call could remove our last ref
    RetainPtr<WebDataSource*> protect(self);
    
    [[self representation] receivedData:data withDataSource:self];
    [[[[self webFrame] frameView] documentView] dataSourceUpdated:self];
}

- (void)_setMainDocumentError:(NSError *)error
{
    if (!toPrivate(_private)->representationFinishedLoading) {
        toPrivate(_private)->representationFinishedLoading = YES;
        [[self representation] receivedError:error withDataSource:self];
    }
}

- (void)_revertToProvisionalState
{
    [self _setRepresentation:nil];
}

+ (NSMutableDictionary *)_repTypesAllowImageTypeOmission:(BOOL)allowImageTypeOmission
{
    static NSMutableDictionary *repTypes = nil;
    static BOOL addedImageTypes = NO;
    
    if (!repTypes) {
        repTypes = [[NSMutableDictionary alloc] init];
        addTypesFromClass(repTypes, [WebHTMLRepresentation class], [WebHTMLRepresentation supportedNonImageMIMETypes]);
        
        // Since this is a "secret default" we don't both registering it.
        BOOL omitPDFSupport = [[NSUserDefaults standardUserDefaults] boolForKey:@"WebKitOmitPDFSupport"];
        if (!omitPDFSupport)
#if PLATFORM(IOS)
#define WebPDFRepresentation ([WebView _getPDFRepresentationClass])
#endif
            addTypesFromClass(repTypes, [WebPDFRepresentation class], [WebPDFRepresentation supportedMIMETypes]);
#if PLATFORM(IOS)
#undef WebPDFRepresentation
#endif
    }
    
    if (!addedImageTypes && !allowImageTypeOmission) {
        addTypesFromClass(repTypes, [WebHTMLRepresentation class], [WebHTMLRepresentation supportedImageMIMETypes]);
        addedImageTypes = YES;
    }
    
    return repTypes;
}

- (void)_replaceSelectionWithArchive:(WebArchive *)archive selectReplacement:(BOOL)selectReplacement
{
    DOMDocumentFragment *fragment = [self _documentFragmentWithArchive:archive];
    if (fragment)
        [[self webFrame] _replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:NO matchStyle:NO];
}

// FIXME: There are few reasons why this method and many of its related methods can't be pushed entirely into WebCore in the future.
- (DOMDocumentFragment *)_documentFragmentWithArchive:(WebArchive *)archive
{
    ASSERT(archive);
    WebResource *mainResource = [archive mainResource];
    if (mainResource) {
        NSString *MIMEType = [mainResource MIMEType];
        if ([WebView canShowMIMETypeAsHTML:MIMEType]) {
            NSString *markupString = [[NSString alloc] initWithData:[mainResource data] encoding:NSUTF8StringEncoding];
            // FIXME: seems poor form to do this as a side effect of getting a document fragment
            if (toPrivate(_private)->loader)
                toPrivate(_private)->loader->addAllArchiveResources([archive _coreLegacyWebArchive]);

            DOMDocumentFragment *fragment = [[self webFrame] _documentFragmentWithMarkupString:markupString baseURLString:[[mainResource URL] _web_originalDataAsString]];
            [markupString release];
            return fragment;
        } else if (MIMETypeRegistry::isSupportedImageMIMEType(MIMEType)) {
            return [self _documentFragmentWithImageResource:mainResource];
            
        }
    }
    return nil;
}

- (DOMDocumentFragment *)_documentFragmentWithImageResource:(WebResource *)resource
{
    DOMElement *imageElement = [self _imageElementWithImageResource:resource];
    if (!imageElement)
        return 0;
    DOMDocumentFragment *fragment = [[[self webFrame] DOMDocument] createDocumentFragment];
    [fragment appendChild:imageElement];
    return fragment;
}

- (DOMElement *)_imageElementWithImageResource:(WebResource *)resource
{
    if (!resource)
        return 0;
    
    [self addSubresource:resource];
    
    DOMElement *imageElement = [[[self webFrame] DOMDocument] createElement:@"img"];
    
    // FIXME: calling _web_originalDataAsString on a file URL returns an absolute path. Workaround this.
    NSURL *URL = [resource URL];
    [imageElement setAttribute:@"src" value:[URL isFileURL] ? [URL absoluteString] : [URL _web_originalDataAsString]];
    
    return imageElement;
}

// May return nil if not initialized with a URL.
- (NSURL *)_URL
{
    const URL& url = toPrivate(_private)->loader->url();
    if (url.isEmpty())
        return nil;
    return url;
}

- (WebView *)_webView
{
    return [[self webFrame] webView];
}

- (BOOL)_isDocumentHTML
{
    NSString *MIMEType = [self _responseMIMEType];
    return [WebView canShowMIMETypeAsHTML:MIMEType];
}

- (void)_makeRepresentation
{
    Class repClass = [[self class] _representationClassForMIMEType:[self _responseMIMEType] allowingPlugins:[[[self _webView] preferences] arePlugInsEnabled]];

#if PLATFORM(IOS)
    if ([repClass respondsToSelector:@selector(_representationClassForWebFrame:)])
        repClass = [repClass performSelector:@selector(_representationClassForWebFrame:) withObject:[self webFrame]];
#endif

    // Check if the data source was already bound?
    if (![[self representation] isKindOfClass:repClass]) {
        id newRep = repClass != nil ? [[repClass alloc] init] : nil;
        [self _setRepresentation:(id <WebDocumentRepresentation>)newRep];
        [newRep release];
    }

    id<WebDocumentRepresentation> representation = toPrivate(_private)->representation.get();
    [representation setDataSource:self];
#if PLATFORM(IOS)
    toPrivate(_private)->loader->setResponseMIMEType([self _responseMIMEType]);
#endif
}

- (DocumentLoader*)_documentLoader
{
    return toPrivate(_private)->loader.get();
}

- (id)_initWithDocumentLoader:(PassRefPtr<WebDocumentLoaderMac>)loader
{
    self = [super init];
    if (!self)
        return nil;

    ASSERT(loader);
    _private = static_cast<void*>(new WebDataSourcePrivate(loader));
        
    LOG(Loading, "creating datasource for %@", static_cast<NSURL *>(toPrivate(_private)->loader->request().url()));

    if ((toPrivate(_private)->includedInWebKitStatistics = [[self webFrame] _isIncludedInWebKitStatistics]))
        ++WebDataSourceCount;

    return self;
}

@end

@implementation WebDataSource

- (instancetype)initWithRequest:(NSURLRequest *)request
{
    return [self _initWithDocumentLoader:WebDocumentLoaderMac::create(request, SubstituteData())];
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebDataSource class], self))
        return;

    if (toPrivate(_private) && toPrivate(_private)->includedInWebKitStatistics)
        --WebDataSourceCount;

#if ENABLE(DISK_IMAGE_CACHE) && PLATFORM(IOS)
    // The code to remove memory mapped notification is only needed when we are viewing a PDF file.
    // In such a case, WebPDFViewPlaceholder sets itself as the dataSourceDelegate. Guard the access
    // to mainResourceData with this nil check so that we avoid assertions due to the resource being
    // made purgeable.
    if (_private && [self dataSourceDelegate]) {
        RefPtr<ResourceBuffer> mainResourceBuffer = toPrivate(_private)->loader->mainResourceData();
        if (mainResourceBuffer) {
            RefPtr<SharedBuffer> mainResourceData = mainResourceBuffer->sharedBuffer();
            if (mainResourceData && 
                mainResourceData->memoryMappedNotificationCallbackData() == self &&
                mainResourceData->memoryMappedNotificationCallback() == BufferMemoryMapped) {
                mainResourceData->setMemoryMappedNotificationCallback(nullptr, nullptr);
            }
        }
    }
#endif
    
#if USE(QUICK_LOOK)
    // Added in -[WebCoreResourceHandleAsDelegate connection:didReceiveResponse:].
    if (NSURL *url = [[self response] URL])
        removeQLPreviewConverterForURL(url);
#endif

    delete toPrivate(_private);

    [super dealloc];
}

- (void)finalize
{
    ASSERT_MAIN_THREAD();

    if (toPrivate(_private) && toPrivate(_private)->includedInWebKitStatistics)
        --WebDataSourceCount;

    delete toPrivate(_private);

    [super finalize];
}

- (NSData *)data
{
    RefPtr<ResourceBuffer> mainResourceData = toPrivate(_private)->loader->mainResourceData();
    if (!mainResourceData)
        return nil;
    return mainResourceData->createNSData().autorelease();
}

- (id <WebDocumentRepresentation>)representation
{
    return toPrivate(_private)->representation.get();
}

- (WebFrame *)webFrame
{
    if (Frame* frame = toPrivate(_private)->loader->frame())
        return kit(frame);

    return nil;
}

- (NSURLRequest *)initialRequest
{
    return toPrivate(_private)->loader->originalRequest().nsURLRequest(UpdateHTTPBody);
}

- (NSMutableURLRequest *)request
{
    FrameLoader* frameLoader = toPrivate(_private)->loader->frameLoader();
    if (!frameLoader || !frameLoader->frameHasLoaded())
        return nil;

    // FIXME: this cast is dubious
    return (NSMutableURLRequest *)toPrivate(_private)->loader->request().nsURLRequest(UpdateHTTPBody);
}

- (NSURLResponse *)response
{
    return toPrivate(_private)->loader->response().nsURLResponse();
}

- (NSString *)textEncodingName
{
    NSString *textEncodingName = toPrivate(_private)->loader->overrideEncoding();
    if (!textEncodingName)
        textEncodingName = [[self response] textEncodingName];
    return textEncodingName;
}

- (BOOL)isLoading
{
    return toPrivate(_private)->loader->isLoadingInAPISense();
}

// Returns nil or the page title.
- (NSString *)pageTitle
{
    return [[self representation] title];
}

- (NSURL *)unreachableURL
{
    const URL& unreachableURL = toPrivate(_private)->loader->unreachableURL();
    if (unreachableURL.isEmpty())
        return nil;
    return unreachableURL;
}

- (WebArchive *)webArchive
{
    // it makes no sense to grab a WebArchive from an uncommitted document.
    if (!toPrivate(_private)->loader->isCommitted())
        return nil;
        
    return [[[WebArchive alloc] _initWithCoreLegacyWebArchive:LegacyWebArchive::create(core([self webFrame]))] autorelease];
}

- (WebResource *)mainResource
{
    RefPtr<ArchiveResource> coreResource = toPrivate(_private)->loader->mainResource();
    return [[[WebResource alloc] _initWithCoreResource:coreResource.release()] autorelease];
}

- (NSArray *)subresources
{
    auto coreSubresources = toPrivate(_private)->loader->subresources();

    auto subresources = adoptNS([[NSMutableArray alloc] initWithCapacity:coreSubresources.size()]);
    for (const auto& coreSubresource : coreSubresources) {
        if (auto resource = adoptNS([[WebResource alloc] _initWithCoreResource:coreSubresource]))
            [subresources addObject:resource.get()];
    }

    return subresources.autorelease();
}

- (WebResource *)subresourceForURL:(NSURL *)URL
{
    RefPtr<ArchiveResource> subresource = toPrivate(_private)->loader->subresource(URL);
    
    return subresource ? [[[WebResource alloc] _initWithCoreResource:subresource.get()] autorelease] : nil;
}

- (void)addSubresource:(WebResource *)subresource
{    
    toPrivate(_private)->loader->addArchiveResource([subresource _coreResource]);
}

@end
