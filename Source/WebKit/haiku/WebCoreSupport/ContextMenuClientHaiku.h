/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ContextMenuClientHaiku_h
#define ContextMenuClientHaiku_h

#include "ContextMenuClient.h"

class BWebPage;

namespace WebCore {

class ContextMenu;

class ContextMenuClientHaiku : public ContextMenuClient {
public:
    ContextMenuClientHaiku(BWebPage*);

    virtual void contextMenuDestroyed() override;

    virtual PlatformMenuDescription getCustomMenuFromDefaultItems(ContextMenu*) override;
    virtual void contextMenuItemSelected(ContextMenuItem*, const ContextMenu*) override;

    virtual void downloadURL(const URL& url) override;
    virtual void lookUpInDictionary(Frame*) override;
    virtual void speak(const String&) override;
    virtual bool isSpeaking() override;
    virtual void stopSpeaking() override;
    virtual void searchWithGoogle(const Frame*) override;

private:
    BWebPage* m_webPage;
};

} // namespace WebCore

#endif // ContextMenuClientHaiku_h

