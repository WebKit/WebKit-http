/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"

#if ENABLE(WEB_AUDIO)

#import "AudioBus.h"

#import "AudioFileReader.h"
#import "AutodrainedPool.h"
#import <wtf/OwnPtr.h>
#import <wtf/PassOwnPtr.h>
#import <Foundation/Foundation.h>

@interface WebCoreAudioBundleClass : NSObject
@end

@implementation WebCoreAudioBundleClass
@end

namespace WebCore {

PassOwnPtr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{
    // This method can be called from other than the main thread, so we need an auto-release pool.
    AutodrainedPool pool;
    
    NSBundle *bundle = [NSBundle bundleForClass:[WebCoreAudioBundleClass class]];
    NSURL *audioFileURL = [bundle URLForResource:[NSString stringWithUTF8String:name] withExtension:@"wav" subdirectory:@"audio"];
#if !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED == 1060
    NSDataReadingOptions options = NSDataReadingMapped;
#else
    NSDataReadingOptions options = NSDataReadingMappedIfSafe;
#endif
    NSData *audioData = [NSData dataWithContentsOfURL:audioFileURL options:options error:nil];

    if (audioData) {
        OwnPtr<AudioBus> bus(createBusFromInMemoryAudioFile([audioData bytes], [audioData length], false, sampleRate));
        return bus.release();
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
