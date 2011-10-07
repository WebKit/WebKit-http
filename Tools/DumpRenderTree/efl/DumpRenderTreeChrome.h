/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DumpRenderTreeChrome_h
#define DumpRenderTreeChrome_h

#include "GCController.h"

#include <Eina.h>
#include <Evas.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

class DumpRenderTreeChrome {
public:
    ~DumpRenderTreeChrome();

    static PassOwnPtr<DumpRenderTreeChrome> create(Evas*);

    Evas_Object* createNewWindow();
    void removeWindow(Evas_Object*);

    Vector<Evas_Object*> extraViews() const;
    Evas_Object* mainFrame() const;
    Evas_Object* mainView() const;

    void resetDefaultsToConsistentValues();

private:
    DumpRenderTreeChrome(Evas*);

    Evas_Object* createView() const;
    bool initialize();

    Evas_Object* m_mainFrame;
    Evas_Object* m_mainView;
    Evas* m_evas;
    OwnPtr<GCController> m_gcController;
    Vector<Evas_Object*> m_extraViews;

    // Smart callbacks
    static void onWindowObjectCleared(void*, Evas_Object*, void*);
    static void onLoadStarted(void*, Evas_Object*, void*);

    static Eina_Bool processWork(void*);

    static void onLoadFinished(void*, Evas_Object*, void*);

    static void onStatusbarTextSet(void*, Evas_Object*, void*);

    static void onTitleChanged(void*, Evas_Object*, void*);

    static void onDocumentLoadFinished(void*, Evas_Object*, void*);
};

#endif // DumpRenderTreeChrome_h
