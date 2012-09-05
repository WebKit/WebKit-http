/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef CCSchedulerStateMachine_h
#define CCSchedulerStateMachine_h

#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// The CCSchedulerStateMachine decides how to coordinate main thread activites
// like painting/running javascript with rendering and input activities on the
// impl thread.
//
// The state machine tracks internal state but is also influenced by external state.
// Internal state includes things like whether a frame has been requested, while
// external state includes things like the current time being near to the vblank time.
//
// The scheduler seperates "what to do next" from the updating of its internal state to
// make testing cleaner.
class CCSchedulerStateMachine {
public:
    CCSchedulerStateMachine();

    enum CommitState {
        COMMIT_STATE_IDLE,
        COMMIT_STATE_FRAME_IN_PROGRESS,
        COMMIT_STATE_UPDATING_RESOURCES,
        COMMIT_STATE_READY_TO_COMMIT,
        COMMIT_STATE_WAITING_FOR_FIRST_DRAW,
    };

    enum TextureState {
        LAYER_TEXTURE_STATE_UNLOCKED,
        LAYER_TEXTURE_STATE_ACQUIRED_BY_MAIN_THREAD,
        LAYER_TEXTURE_STATE_ACQUIRED_BY_IMPL_THREAD,
    };

    enum ContextState {
        CONTEXT_ACTIVE,
        CONTEXT_LOST,
        CONTEXT_RECREATING,
    };

    bool commitPending() const
    {
        return m_commitState != COMMIT_STATE_IDLE;
    }

    bool redrawPending() const { return m_needsRedraw; }

    enum Action {
        ACTION_NONE,
        ACTION_BEGIN_FRAME,
        ACTION_BEGIN_UPDATE_MORE_RESOURCES,
        ACTION_COMMIT,
        ACTION_DRAW_IF_POSSIBLE,
        ACTION_DRAW_FORCED,
        ACTION_BEGIN_CONTEXT_RECREATION,
        ACTION_ACQUIRE_LAYER_TEXTURES_FOR_MAIN_THREAD,
    };
    Action nextAction() const;
    void updateState(Action);

    // Indicates whether the scheduler needs a vsync callback in order to make
    // progress.
    bool vsyncCallbackNeeded() const;

    // Indicates that the system has entered and left a vsync callback.
    // The scheduler will not draw more than once in a given vsync callback.
    void didEnterVSync();
    void didLeaveVSync();

    // Indicates whether the LayerTreeHostImpl is visible.
    void setVisible(bool);

    // Indicates that a redraw is required, either due to the impl tree changing
    // or the screen being damaged and simply needing redisplay.
    void setNeedsRedraw();

    // As setNeedsRedraw(), but ensures the draw will definitely happen even if
    // we are not visible.
    void setNeedsForcedRedraw();

    // Indicates whether ACTION_DRAW_IF_POSSIBLE drew to the screen or not.
    void didDrawIfPossibleCompleted(bool success);

    // Indicates that a new commit flow needs to be performed, either to pull
    // updates from the main thread to the impl, or to push deltas from the impl
    // thread to main.
    void setNeedsCommit();

    // As setNeedsCommit(), but ensures the beginFrame will definitely happen even if
    // we are not visible.
    void setNeedsForcedCommit();

    // Call this only in response to receiving an ACTION_BEGIN_FRAME
    // from nextState. Indicates that all painting is complete and that
    // updating of compositor resources can begin.
    void beginFrameComplete();

    // Call this only in response to receiving an ACTION_BEGIN_FRAME
    // from nextState if the client rejects the beginFrame message.
    void beginFrameAborted();

    // Call this only in response to receiving an ACTION_UPDATE_MORE_RESOURCES
    // from nextState. Indicates that the specific update request completed.
    void beginUpdateMoreResourcesComplete(bool morePending);

    // Request exclusive access to the textures that back single buffered
    // layers on behalf of the main thread. Upon acqusition,
    // ACTION_DRAW_IF_POSSIBLE will not draw until the main thread releases the
    // textures to the impl thread by committing the layers.
    void setMainThreadNeedsLayerTextures();

    // Indicates whether we can successfully begin a frame at this time.
    void setCanBeginFrame(bool can) { m_canBeginFrame = can; }

    // Indicates whether drawing would, at this time, make sense.
    // canDraw can be used to supress flashes or checkerboarding
    // when such behavior would be undesirable.
    void setCanDraw(bool can) { m_canDraw = can; }

    void didLoseContext();
    void didRecreateContext();

    // Exposed for testing purposes.
    void setMaximumNumberOfFailedDrawsBeforeDrawIsForced(int);

    String toString();

protected:
    bool shouldDrawForced() const;
    bool drawSuspendedUntilCommit() const;
    bool scheduledToDraw() const;
    bool shouldDraw() const;
    bool shouldAcquireLayerTexturesForMainThread() const;
    bool hasDrawnThisFrame() const;

    CommitState m_commitState;

    int m_currentFrameNumber;
    int m_lastFrameNumberWhereDrawWasCalled;
    int m_consecutiveFailedDraws;
    int m_maximumNumberOfFailedDrawsBeforeDrawIsForced;
    bool m_needsRedraw;
    bool m_needsForcedRedraw;
    bool m_needsForcedRedrawAfterNextCommit;
    bool m_needsCommit;
    bool m_needsForcedCommit;
    bool m_mainThreadNeedsLayerTextures;
    bool m_updateMoreResourcesPending;
    bool m_insideVSync;
    bool m_visible;
    bool m_canBeginFrame;
    bool m_canDraw;
    bool m_drawIfPossibleFailed;
    TextureState m_textureState;
    ContextState m_contextState;
};

}

#endif // CCSchedulerStateMachine_h
