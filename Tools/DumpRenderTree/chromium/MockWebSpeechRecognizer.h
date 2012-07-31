/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef MockWebSpeechRecognizer_h
#define MockWebSpeechRecognizer_h

#if ENABLE(SCRIPTED_SPEECH)

#include "Task.h"
#include "WebSpeechRecognizer.h"
#include <wtf/Compiler.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebKit {
class WebSpeechRecognitionHandle;
class WebSpeechRecognitionParams;
class WebSpeechRecognizerClient;
}

class MockWebSpeechRecognizer : public WebKit::WebSpeechRecognizer {
public:
    static PassOwnPtr<MockWebSpeechRecognizer> create();
    ~MockWebSpeechRecognizer();

    // WebSpeechRecognizer implementation:
    virtual void start(const WebKit::WebSpeechRecognitionHandle&, const WebKit::WebSpeechRecognitionParams&, WebKit::WebSpeechRecognizerClient*) OVERRIDE;
    virtual void stop(const WebKit::WebSpeechRecognitionHandle&, WebKit::WebSpeechRecognizerClient*) OVERRIDE;
    virtual void abort(const WebKit::WebSpeechRecognitionHandle&, WebKit::WebSpeechRecognizerClient*) OVERRIDE;

    // Methods accessed by layout tests:
    void addMockResult(const WebKit::WebString& transcript, float confidence);
    void setError(int code, const WebKit::WebString& message);
    bool wasAborted() const { return m_wasAborted; }

    // Methods accessed from Task objects:
    WebKit::WebSpeechRecognizerClient* client() { return m_client; }
    WebKit::WebSpeechRecognitionHandle& handle() { return m_handle; }
    TaskList* taskList() { return &m_taskList; }

    class Task {
    public:
        Task(MockWebSpeechRecognizer* recognizer) : m_recognizer(recognizer) { }
        virtual ~Task() { }
        virtual void run() = 0;
    protected:
        MockWebSpeechRecognizer* m_recognizer;
    };

private:
    MockWebSpeechRecognizer();
    void startTaskQueue();
    void clearTaskQueue();

    TaskList m_taskList;
    WebKit::WebSpeechRecognitionHandle m_handle;
    WebKit::WebSpeechRecognizerClient* m_client;
    Vector<WebKit::WebString> m_mockTranscripts;
    Vector<float> m_mockConfidences;
    bool m_wasAborted;

    // Queue of tasks to be run.
    Vector<OwnPtr<Task> > m_taskQueue;
    bool m_taskQueueRunning;

    // Task for stepping the queue.
    class StepTask : public MethodTask<MockWebSpeechRecognizer> {
    public:
        StepTask(MockWebSpeechRecognizer* object) : MethodTask<MockWebSpeechRecognizer>(object) { }
        virtual void runIfValid() OVERRIDE;
    };
};

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // MockWebSpeechRecognizer_h
