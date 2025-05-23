/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NUPLAYER_RENDERER_H_

#define NUPLAYER_RENDERER_H_

#include <atomic>

#include <media/AudioResamplerPublic.h>
#include <media/AVSyncSettings.h>

#include "NuPlayer.h"

namespace android {

class  AWakeLock;
struct MediaClock;
class MediaCodecBuffer;
struct VideoFrameSchedulerBase;

struct NuPlayer::Renderer : public AHandler {
    enum Flags {
        FLAG_REAL_TIME = 1,
        FLAG_OFFLOAD_AUDIO = 2,
    };
    Renderer(const sp<MediaPlayerBase::AudioSink> &sink,
             const sp<MediaClock> &mediaClock,
             const sp<AMessage> &notify,
             uint32_t flags = 0);

    static size_t AudioSinkCallback(
            MediaPlayerBase::AudioSink *audioSink,
            void *data, size_t size, void *me,
            MediaPlayerBase::AudioSink::cb_event_t event);

    void queueBuffer(
            bool audio,
            const sp<MediaCodecBuffer> &buffer,
            const sp<AMessage> &notifyConsumed);

    void queueEOS(bool audio, status_t finalResult);

    status_t setPlaybackSettings(const AudioPlaybackRate &rate /* sanitized */);
    status_t getPlaybackSettings(AudioPlaybackRate *rate /* nonnull */);
    status_t setSyncSettings(const AVSyncSettings &sync, float videoFpsHint);
    status_t getSyncSettings(AVSyncSettings *sync /* nonnull */, float *videoFps /* nonnull */);

    void flush(bool audio, bool notifyComplete);

    void signalTimeDiscontinuity();

    void signalDisableOffloadAudio();
    void signalEnableOffloadAudio();

    void pause(bool forPreroll = false);
    void resume();

    void setVideoFrameRate(float fps);
    void setIsSeekonPause();

    status_t getCurrentPosition(int64_t *mediaUs);
    int64_t getVideoLateByUs();

    bool isVideoPrerollInprogress() const;

    status_t openAudioSink(
            const sp<AMessage> &format,
            bool offloadOnly,
            bool hasVideo,
            uint32_t flags,
            bool *isOffloaded,
            bool isStreaming);
    void closeAudioSink();

    void dump(AString& logString);

    // re-open audio sink after all pending audio buffers played.
    void changeAudioFormat(
            const sp<AMessage> &format,
            bool offloadOnly,
            bool hasVideo,
            uint32_t flags,
            bool isStreaming,
            const sp<AMessage> &notify);

    enum {
        kWhatEOS                      = 'eos ',
        kWhatFlushComplete            = 'fluC',
        kWhatPosition                 = 'posi',
        kWhatVideoRenderingStart      = 'vdrd',
        kWhatMediaRenderingStart      = 'mdrd',
        kWhatAudioTearDown            = 'adTD',
        kWhatAudioOffloadPauseTimeout = 'aOPT',
        kWhatVideoPrerollComplete     = 'vdpC',
        kWhatReleaseWakeLock          = 'adRL',
    };

    enum AudioTearDownReason {
        kDueToError = 0,   // Could restart with either offload or non-offload.
        kDueToTimeout,
        kForceNonOffload,  // Restart only with non-offload.
    };

protected:
    virtual ~Renderer();

    virtual void onMessageReceived(const sp<AMessage> &msg);

    enum {
        kWhatDrainAudioQueue     = 'draA',
        kWhatDrainVideoQueue     = 'draV',
        kWhatPostDrainVideoQueue = 'pDVQ',
        kWhatQueueBuffer         = 'queB',
        kWhatQueueEOS            = 'qEOS',
        kWhatConfigPlayback      = 'cfPB',
        kWhatConfigSync          = 'cfSy',
        kWhatGetPlaybackSettings = 'gPbS',
        kWhatGetSyncSettings     = 'gSyS',
        kWhatFlush               = 'flus',
        kWhatPause               = 'paus',
        kWhatResume              = 'resm',
        kWhatOpenAudioSink       = 'opnA',
        kWhatCloseAudioSink      = 'clsA',
        kWhatChangeAudioFormat   = 'chgA',
        kWhatStopAudioSink       = 'stpA',
        kWhatDisableOffloadAudio = 'noOA',
        kWhatEnableOffloadAudio  = 'enOA',
        kWhatSetVideoFrameRate   = 'sVFR',
    };

    // if mBuffer != nullptr, it's a buffer containing real data.
    // else if mNotifyConsumed == nullptr, it's EOS.
    // else it's a tag for re-opening audio sink in different format.
    struct QueueEntry {
        sp<MediaCodecBuffer> mBuffer;
        sp<AMessage> mMeta;
        sp<AMessage> mNotifyConsumed;
        size_t mOffset;
        status_t mFinalResult;
        int32_t mBufferOrdinal;
    };

    static const int64_t kMinPositionUpdateDelayUs;

    sp<MediaPlayerBase::AudioSink> mAudioSink;
    bool mUseVirtualAudioSink;
    sp<AMessage> mNotify;
    Mutex mLock;
    uint32_t mFlags;
    List<QueueEntry> mAudioQueue;
    List<QueueEntry> mVideoQueue;
    uint32_t mNumFramesWritten;
    sp<VideoFrameSchedulerBase> mVideoScheduler;

    bool mDrainAudioQueuePending;
    bool mDrainVideoQueuePending;
    int32_t mAudioQueueGeneration;
    int32_t mVideoQueueGeneration;
    int32_t mAudioDrainGeneration;
    int32_t mVideoDrainGeneration;
    int32_t mAudioEOSGeneration;

    const sp<MediaClock> mMediaClock;
    float mPlaybackRate; // audio track rate

    AudioPlaybackRate mPlaybackSettings;
    AVSyncSettings mSyncSettings;
    float mVideoFpsHint;

    int64_t mLastAudioAnchorNowUs;
    int64_t mAudioFirstAnchorTimeMediaUs;
    // previous audio anchor timestamp, in media time base.
    int64_t mAudioAnchorTimeMediaUs;
    // previous anchor timestamp (audio or video), in media time base.
    int64_t mAnchorTimeMediaUs;
    int64_t mAnchorNumFramesWritten;
    int64_t mVideoLateByUs;
    int64_t mNextVideoTimeMediaUs;
    bool mHasAudio;
    bool mHasVideo;

    bool mNotifyCompleteAudio;
    bool mNotifyCompleteVideo;

    bool mSyncQueues;

    // modified on only renderer's thread.
    bool mPaused;
    int64_t mPauseDrainAudioAllowedUs; // time when we can drain/deliver audio in pause mode.

    bool mVideoPrerollInprogress;
    bool mVideoSampleReceived;
    bool mVideoRenderingStarted;
    int32_t mVideoRenderingStartGeneration;
    int32_t mAudioRenderingStartGeneration;
    bool mRenderingDataDelivered;

    int64_t mNextAudioClockUpdateTimeUs;
    // the media timestamp of last audio sample right before EOS.
    int64_t mLastAudioMediaTimeUs;

    int32_t mAudioOffloadPauseTimeoutGeneration;
    bool mAudioTornDown;
    audio_offload_info_t mCurrentOffloadInfo;

    struct PcmInfo {
        audio_channel_mask_t mChannelMask;
        audio_output_flags_t mFlags;
        audio_format_t mFormat;
        int32_t mNumChannels;
        int32_t mSampleRate;
    };
    PcmInfo mCurrentPcmInfo;
    static const PcmInfo AUDIO_PCMINFO_INITIALIZER;

    int32_t mTotalBuffersQueued;
    int32_t mLastAudioBufferDrained;
    bool mUseAudioCallback;

    sp<AWakeLock> mWakeLock;

    std::atomic_flag mSyncFlag = ATOMIC_FLAG_INIT;
    Mutex mSyncLock;
    Condition mSyncCondition;
    int64_t mSyncCount{0};

    status_t getCurrentPositionOnLooper(int64_t *mediaUs);
    status_t getCurrentPositionOnLooper(
            int64_t *mediaUs, int64_t nowUs, bool allowPastQueuedVideo = false);
    bool getCurrentPositionIfPaused_l(int64_t *mediaUs);
    status_t getCurrentPositionFromAnchor(
            int64_t *mediaUs, int64_t nowUs, bool allowPastQueuedVideo = false);

    struct WakeLockEvent{
        int64_t mTimeMs;
        int32_t mEventTimeoutGeneration;
        int32_t mRendererTimeoutGeneration;

        WakeLockEvent():
            mTimeMs(0),
            mEventTimeoutGeneration(0),
            mRendererTimeoutGeneration(0) {}

        void updateValues(int64_t timeMs,
                          int32_t eventGeneration,
                          int32_t rendererGeneration) {
            mTimeMs = timeMs;
            mEventTimeoutGeneration = eventGeneration;
            mRendererTimeoutGeneration = rendererGeneration;
        }

        void dump(AString& logString);
    };

    WakeLockEvent mWakelockAcquireEvent;
    WakeLockEvent mWakelockTimeoutEvent;
    WakeLockEvent mWakelockReleaseEvent;
    WakeLockEvent mWakelockCancelEvent;

    void notifyEOSCallback();
    size_t fillAudioBuffer(void *buffer, size_t size);

    bool onDrainAudioQueue();
    void drainAudioQueueUntilLastEOS();
    int64_t getPendingAudioPlayoutDurationUs(int64_t nowUs);
    void postDrainAudioQueue_l(int64_t delayUs = 0);

    void clearAnchorTime();
    void clearAudioFirstAnchorTime_l();
    void setAudioFirstAnchorTimeIfNeeded_l(int64_t mediaUs);
    void setVideoLateByUs(int64_t lateUs);

    void onNewAudioMediaTime(int64_t mediaTimeUs);
    int64_t getRealTimeUs(int64_t mediaTimeUs, int64_t nowUs);

    void onDrainVideoQueue();
    void postDrainVideoQueue();

    void forceAudioUpdateAnchorTime();

    void prepareForMediaRenderingStart_l();
    void notifyIfMediaRenderingStarted_l();

    void onQueueBuffer(const sp<AMessage> &msg);
    void onQueueEOS(const sp<AMessage> &msg);
    void onFlush(const sp<AMessage> &msg);
    void onAudioSinkChanged();
    void onDisableOffloadAudio();
    void onEnableOffloadAudio();
    status_t onConfigPlayback(const AudioPlaybackRate &rate /* sanitized */);
    status_t onGetPlaybackSettings(AudioPlaybackRate *rate /* nonnull */);
    status_t onConfigSync(const AVSyncSettings &sync, float videoFpsHint);
    status_t onGetSyncSettings(AVSyncSettings *sync /* nonnull */, float *videoFps /* nonnull */);

    void onPause(bool forPreroll = false);
    void onResume();
    void onSetVideoFrameRate(float fps);
    int32_t getQueueGeneration(bool audio);
    int32_t getDrainGeneration(bool audio);
    bool getSyncQueues();
    void onAudioTearDown(AudioTearDownReason reason);
    virtual status_t onOpenAudioSink(
            const sp<AMessage> &format,
            bool offloadOnly,
            bool hasVideo,
            uint32_t flags,
            bool isStreaming);
    void onCloseAudioSink();
    void onChangeAudioFormat(const sp<AMessage> &meta, const sp<AMessage> &notify);

    void notifyEOS(bool audio, status_t finalResult, int64_t delayUs = 0);
    void notifyEOS_l(bool audio, status_t finalResult, int64_t delayUs = 0);
    void notifyFlushComplete(bool audio);
    void notifyPosition();
    void notifyVideoLateBy(int64_t lateByUs);
    void notifyVideoRenderingStart();
    void notifyAudioTearDown(AudioTearDownReason reason);

    void flushQueue(List<QueueEntry> *queue);
    bool dropBufferIfStale(bool audio, const sp<AMessage> &msg);
    void syncQueuesDone_l();

    bool offloadingAudio() const { return (mFlags & FLAG_OFFLOAD_AUDIO) != 0; }

    void startAudioOffloadPauseTimeout();
    void cancelAudioOffloadPauseTimeout();

    int64_t getDurationUsIfPlayedAtSampleRate(uint32_t numFrames);

    DISALLOW_EVIL_CONSTRUCTORS(Renderer);

private:
    bool mNeedVideoClearAnchor;
    bool mIsSeekonPause;
    float mVideoRenderFps;
};

} // namespace android

#endif  // NUPLAYER_RENDERER_H_
