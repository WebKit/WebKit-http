/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MediaPlayerPrivateQt_h
#define MediaPlayerPrivateQt_h

#include "MediaPlayerPrivate.h"

#include <QAbstractVideoSurface>
#include <QMediaPlayer>
#include <QObject>
#include <QVideoSurfaceFormat>

QT_BEGIN_NAMESPACE
class QMediaPlayerControl;
class QGraphicsVideoItem;
class QGraphicsScene;
QT_END_NAMESPACE

#include "TextureMapperPlatformLayer.h"

namespace WebCore {

class MediaPlayerPrivateQt : public QAbstractVideoSurface, public MediaPlayerPrivateInterface
                           , public TextureMapperPlatformLayer
{

    Q_OBJECT

public:
    static std::unique_ptr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    explicit MediaPlayerPrivateQt(MediaPlayer*);
    ~MediaPlayerPrivateQt();

    static void registerMediaEngine(MediaEngineRegistrar);
    static void getSupportedTypes(HashSet<WTF::String, WTF::ASCIICaseInsensitiveHash>&);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters& parameters);
    static bool isAvailable() { return true; }

    bool hasVideo() const override;
    bool hasAudio() const override;

    void load(const String &url) override;
    void commitLoad(const String& url);
    void resumeLoad();
    void cancelLoad() override;

    void play() override;
    void pause() override;
    void prepareToPlay() override;

    bool paused() const override;
    bool seeking() const override;

    float duration() const override;
    float currentTime() const override;
    void seek(float) override;

    void setRate(float) override;
    void setVolume(float) override;

    bool supportsMuting() const override;
    void setMuted(bool) override;

    void setPreload(MediaPlayer::Preload) override;

    MediaPlayer::NetworkState networkState() const override;
    MediaPlayer::ReadyState readyState() const override;

    std::unique_ptr<PlatformTimeRanges> buffered() const override;
    float maxTimeSeekable() const override;
    bool didLoadingProgress() const override;
    unsigned long long totalBytes() const override;

    void setVisible(bool) override;

    FloatSize naturalSize() const override;
    void setSize(const IntSize&) override;

    void paint(GraphicsContext&, const FloatRect&) override;
    // reimplemented for canvas drawImage(HTMLVideoElement)
    void paintCurrentFrameInContext(GraphicsContext&, const FloatRect&) override;

    bool supportsFullscreen() const override { return true; }

    // whether accelerated rendering is supported by the media engine for the current media.
    bool supportsAcceleratedRendering() const override { return false; }
    // called when the rendering system flips the into or out of accelerated rendering mode.
    void acceleratedRenderingStateChanged() override { }
    // Const-casting here is safe, since all of TextureMapperPlatformLayer's functions are const.g
    PlatformLayer* platformLayer() const override { return 0; }
    void paintToTextureMapper(TextureMapper&, const FloatRect& targetRect, const TransformationMatrix&, float opacity) override;

    PlatformMedia platformMedia() const override;

    QMediaPlayer* mediaPlayer() const { return m_mediaPlayer; }
    void removeVideoItem();
    void restoreVideoItem();

    // QAbstractVideoSurface methods
    bool start(const QVideoSurfaceFormat&) override;
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType = QAbstractVideoBuffer::NoHandle) const override;
    bool present(const QVideoFrame&) override;

private Q_SLOTS:
    void mediaStatusChanged(QMediaPlayer::MediaStatus);
    void handleError(QMediaPlayer::Error);
    void stateChanged(QMediaPlayer::State);
    void surfaceFormatChanged(const QVideoSurfaceFormat&);
    void positionChanged(qint64);
    void durationChanged(qint64);
    void bufferStatusChanged(int);
    void volumeChanged(int);
    void mutedChanged(bool);

private:
    void updateStates();

    String engineDescription() const override { return "Qt"; }

private:
    MediaPlayer* m_webCorePlayer;
    QMediaPlayer* m_mediaPlayer;
    QMediaPlayerControl* m_mediaPlayerControl;
    QVideoSurfaceFormat m_frameFormat;
    QVideoFrame m_currentVideoFrame;

    mutable MediaPlayer::NetworkState m_networkState;
    mutable MediaPlayer::ReadyState m_readyState;

    IntSize m_currentSize;
    IntSize m_naturalSize;
    bool m_isVisible;
    bool m_isSeeking;
    bool m_composited;
    MediaPlayer::Preload m_preload;
    mutable unsigned m_bytesLoadedAtLastDidLoadingProgress;
    bool m_delayingLoad;
    String m_mediaUrl;
    bool m_suppressNextPlaybackChanged;
    bool m_prerolling;

};
}

#endif // MediaPlayerPrivateQt_h
