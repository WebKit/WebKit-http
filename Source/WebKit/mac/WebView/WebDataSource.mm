/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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
#import <WebCore/KURL.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreURLResponse.h>
#import <WebKit/DOMHTML.h>
#import <WebKit/DOMPrivate.h>
#import <runtime/InitializeThreading.h>
#import <wtf/Assertions.h>
#import <wtf/MainThread.h>

using namespace WebCore;

@interface WebDataSourcePrivate : NSObject {
@public
    WebDocumentLoaderMac* loader;
   
    id <WebDocumentRepresentation> representation;
    
    BOOL representationFinishedLoading;
    BOOL includedInWebKitStatistics;
}
@end

@implementation WebDataSourcePrivate 

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCoreObjCFinalizeOnMainThread(self);
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebDataSourcePrivate class], self))
        return;

    ASSERT(loader);
    if (loader) {
        ASSERT(!loader->isLoading());
        loader->detachDataSource();
        loader->deref();
    }
    
    [representation release];

    [super dealloc];
}

- (void)finalize
{
    ASSERT_MAIN_THREAD();

    ASSERT(loader);
    if (loader) {
        ASSERT(!loader->isLoading());
        loader->detachDataSource();
        loader->deref();
    }

    [super finalize];
}

@end

@interface WebDataSource (WebFileInternal)
@end

@implementation WebDataSource (WebFileInternal)

- (void)_setRepresentation:(id<WebDocumentRepresentation>)representation
{
    [_private->representation release];
    _private->representation = [representation retain];
    _private->representationFinishedLoading = NO;
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

- (NSError *)_mainDocumentError
{
    return _private->loader->mainDocumentError();
}

- (void)_addSubframeArchives:(NSArray *)subframeArchives
{
    // FIXME: This SPI is poor, poor design.  Can we come up with another solution for those who need it?
    DocumentLoader* loader = [self _documentLoader];
    ASSERT(loader);
    
    NSEnumerator *enumerator = [subframeArchives objectEnumerator];
    WebArchive *archive;
    while ((archive = [enumerator nextObject]) != nil)
        loader->addAllArchiveResources([archive _coreLegacyWebArchive]);
}

- (NSFileWrapper *)_fileWrapperForURL:(NSURL *)URL
{
    if ([URL isFileURL]) {
        NSString *path = [[URL path] stringByResolvingSymlinksInPath];
        return [[[NSFileWrapper alloc] initWithPath:path] autorelease];
    }
    
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

- (NSString *)_responseMIMEType
{
    return [[self response] MIMEType];
}

- (BOOL)_transferApplicationCache:(NSString*)destinationBundleIdentifier
{
    DocumentLoader* loader = [self _documentLoader];
    
    if (!loader)
        return NO;
        
    NSString *cacheDir = [NSString _webkit_localCacheDirectoryWithBundleIdentifier:destinationBundleIdentifier];
    
    return ApplicationCacheStorage::storeCopyOfCache(cacheDir, loader->applicationCacheHost());
}

- (void)_setDeferMainResourceDataLoad:(BOOL)flag
{
    DocumentLoader* loader = [self _documentLoader];

    if (!loader)
        return;

    loader->setDeferMainResourceDataLoad(flag);
}

@end

@implementation WebDataSource (WebInternal)

- (void)_finishedLoading
{
    _private->representationFinishedLoading = YES;
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
    if (!_private->representationFinishedLoading) {
        _private->representationFinishedLoading = YES;
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
            addTypesFromClass(repTypes, [WebPDFRepresentation class], [WebPDFRepresentation supportedMIMETypes]);
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
            if (DocumentLoader* loader = [self _documentLoader])
                loader->addAllArchiveResources([archive _coreLegacyWebArchive]);

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
    const KURL& url = _private->loader->url();
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
    
    // Check if the data source was already bound?
    if (![[self representation] isKindOfClass:repClass]) {
        id newRep = repClass != nil ? [[repClass alloc] init] : nil;
        [self _setRepresentation:(id <WebDocumentRepresentation>)newRep];
        [newRep release];
    }

    [_private->representation setDataSource:self];
}

- (DocumentLoader*)_documentLoader
{
    return _private->loader;
}

- (id)_initWithDocumentLoader:(PassRefPtr<WebDocumentLoaderMac>)loader
{
    self = [super init];
    if (!self)
        return nil;
    
    _private = [[WebDataSourcePrivate alloc] init];
    
    _private->loader = loader.leakRef();
        
    LOG(Loading, "creating datasource for %@", static_cast<NSURL *>(_private->loader->request().url()));

    if ((_private->includedInWebKitStatistics = [[self webFrame] _isIncludedInWebKitStatistics]))
        ++WebDataSourceCount;

    return self;
}

@end

@implementation WebDataSource

- (id)initWithRequest:(NSURLRequest *)request
{
    return [self _initWithDocumentLoader:WebDocumentLoaderMac::create(request, SubstituteData())];
}

- (void)dealloc
{
    if (_private && _private->includedInWebKitStatistics)
        --WebDataSourceCount;

    [_private release];

    [super dealloc];
}

- (void)finalize
{
    if (_private && _private->includedInWebKitStatistics)
        --WebDataSourceCount;

    [super finalize];
}

- (NSData *)data
{
    RefPtr<SharedBuffer> mainResourceData = _private->loader->mainResourceData();
    if (!mainResourceData)
        return nil;
    return [mainResourceData->createNSData() autorelease];
}

- (id <WebDocumentRepresentation>)representation
{
    return _private->representation;
}

- (WebFrame *)webFrame
{
    FrameLoader* frameLoader = _private->loader->frameLoader();
    if (!frameLoader)
        return nil;
    return static_cast<WebFrameLoaderClient*>(frameLoader->client())->webFrame();
}

- (NSURLRequest *)initialRequest
{
    return _private->loader->originalRequest().nsURLRequest();
}

- (NSMutableURLRequest *)request
{
    FrameLoader* frameLoader = _private->loader->frameLoader();
    if (!frameLoader || !frameLoader->frameHasLoaded())
        return nil;

    // FIXME: this cast is dubious
    return (NSMutableURLRequest *)_private->loader->request().nsURLRequest();
}

- (NSURLResponse *)response
{
    return _private->loader->response().nsURLResponse();
}

- (NSString *)textEncodingName
{
    NSString *textEncodingName = _private->loader->overrideEncoding();
    if (!textEncodingName)
        textEncodingName = [[self response] textEncodingName];
    return textEncodingName;
}

- (BOOL)isLoading
{
    return _private->loader->isLoadingInAPISense();
}

// Returns nil or the page title.
- (NSString *)pageTitle
{
    return [[self representation] title];
}

- (NSURL *)unreachableURL
{
    const KURL& unreachableURL = _private->loader->unreachableURL();
    if (unreachableURL.isEmpty())
        return nil;
    return unreachableURL;
}

- (WebArchive *)webArchive
{
    // it makes no sense to grab a WebArchive from an uncommitted document.
    if (!_private->loader->isCommitted())
        return nil;
        
    return [[[WebArchive alloc] _initWithCoreLegacyWebArchive:LegacyWebArchive::create(core([self webFrame]))] autorelease];
}

- (WebResource *)mainResource
{
    RefPtr<ArchiveResource> coreResource = _private->loader->mainResource();
    return [[[WebResource alloc] _initWithCoreResource:coreResource.release()] autorelease];
}

- (NSArray *)subresources
{
    Vector<PassRefPtr<ArchiveResource> > coreSubresources;
    _private->loader->getSubresources(coreSubresources);

    NSMutableArray *subresources = [[NSMutableArray alloc] initWithCapacity:coreSubresources.size()];
    for (unsigned i = 0; i < coreSubresources.size(); ++i) {
        WebResource *resource = [[WebResource alloc] _initWithCoreResource:coreSubresources[i]];
        if (resource) {
            [subresources addObject:resource];
            [resource release];
        }
    }

    return [subresources autorelease];
}

- (WebResource *)subresourceForURL:(NSURL *)URL
{
    RefPtr<ArchiveResource> subresource = _private->loader->subresource(URL);
    
    return subresource ? [[[WebResource alloc] _initWithCoreResource:subresource.get()] autorelease] : nil;
}

- (void)addSubresource:(WebResource *)subresource
{    
    _private->loader->addArchiveResource([subresource _coreResource]);
}

@end
