/**
 * @file llwebrtc_impl.h
 * @brief WebRTC interface implementation header
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LLWEBRTC_IMPL_H
#define LLWEBRTC_IMPL_H

#define LL_MAKEDLL
#if defined(_WIN32) || defined(_WIN64)
#define WEBRTC_WIN 1
#elif defined(__APPLE__)
#define WEBRTC_MAC 1
#define WEBRTC_POSIX 1
#elif __linux__
#define WEBRTC_LINUX 1
#endif

#include "llwebrtc.h"
// WebRTC Includes
#ifdef WEBRTC_WIN
#pragma warning(disable : 4996)
#pragma warning(disable : 4068)
#endif // WEBRTC_WIN

#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"
#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "api/create_peerconnection_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_data_observer.h"
#include "rtc_base/task_queue.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_defines.h"


namespace llwebrtc
{

class LLWebRTCPeerConnectionImpl;

class LLAudioDeviceObserver : public webrtc::AudioDeviceDataObserver
{
  public:
    LLAudioDeviceObserver();

    float getMicrophoneEnergy();

    void OnCaptureData(const void    *audio_samples,
                       const size_t   num_samples,
                       const size_t   bytes_per_sample,
                       const size_t   num_channels,
                       const uint32_t samples_per_sec) override;

    void OnRenderData(const void    *audio_samples,
                      const size_t   num_samples,
                      const size_t   bytes_per_sample,
                      const size_t   num_channels,
                      const uint32_t samples_per_sec) override;

  protected:
    float mSumVector[30];  // 300 ms of smoothing
    float mMicrophoneEnergy;
};

class LLWebRTCImpl : public LLWebRTCDeviceInterface
{
  public:
    LLWebRTCImpl() : 
        mTuningAudioDeviceObserver(nullptr), mPeerAudioDeviceObserver(nullptr), mMute(true)
    {
    }
    ~LLWebRTCImpl() {}

    void init();
    void terminate();

    //
    // LLWebRTCDeviceInterface
    //

    void refreshDevices() override;

    void setDevicesObserver(LLWebRTCDevicesObserver *observer) override;
    void unsetDevicesObserver(LLWebRTCDevicesObserver *observer) override;

    void setCaptureDevice(const std::string& id) override;
    void setRenderDevice(const std::string& id) override;

    void setTuningMode(bool enable) override;
    float getTuningAudioLevel() override;
    float getPeerAudioLevel() override;

    void setSpeakerVolume(float volume) override; // range 0.0-1.0
    void setMicrophoneVolume(float volume) override; // range 0.0-1.0  
    void setMute(bool mute) override;
    
    //
    // Helpers
    //

    void PostWorkerTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mWorkerThread->PostTask(std::move(task), location);
    }
    
    void PostSignalingTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mSignalingThread->PostTask(std::move(task), location);
    }

    void PostNetworkTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mNetworkThread->PostTask(std::move(task), location);
    }

    void WorkerBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mWorkerThread->BlockingCall(std::move(functor), location);
    }
    
    void SignalingBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mSignalingThread->BlockingCall(std::move(functor), location);
    }

    void NetworkBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mNetworkThread->BlockingCall(std::move(functor), location);
    }
    
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPeerConnectionFactory() { return mPeerConnectionFactory; }

    LLWebRTCPeerConnection *  newPeerConnection();
    void freePeerConnection(LLWebRTCPeerConnection * peer_connection);

  protected:

    std::unique_ptr<rtc::Thread>                               mNetworkThread;
    std::unique_ptr<rtc::Thread>                               mWorkerThread;
    std::unique_ptr<rtc::Thread>                               mSignalingThread;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;
    webrtc::PeerConnectionInterface::RTCConfiguration          mConfiguration;
    std::unique_ptr<webrtc::TaskQueueFactory>                  mTaskQueueFactory;


    // Devices
    void updateDevices();
    rtc::scoped_refptr<webrtc::AudioDeviceModule>              mTuningDeviceModule;
    rtc::scoped_refptr<webrtc::AudioDeviceModule>              mPeerDeviceModule;
    std::vector<LLWebRTCDevicesObserver *>                     mVoiceDevicesObserverList;

    // accessors in webrtc aren't apparently implemented yet.
    int32_t                                                    mPlayoutDevice;
    int32_t                                                    mRecordingDevice;
    bool                                                       mMute;

    LLAudioDeviceObserver *                                    mTuningAudioDeviceObserver;
    LLAudioDeviceObserver *                                    mPeerAudioDeviceObserver;
    
    // peer connections
    std::vector<rtc::scoped_refptr<LLWebRTCPeerConnectionImpl>>     mPeerConnections;
};

class LLWebRTCPeerConnectionImpl : public LLWebRTCPeerConnection,
                                   public LLWebRTCAudioInterface,
                                   public LLWebRTCDataInterface,
                                   public webrtc::PeerConnectionObserver,
                                   public webrtc::CreateSessionDescriptionObserver,
                                   public webrtc::SetRemoteDescriptionObserverInterface,
                                   public webrtc::SetLocalDescriptionObserverInterface,
                                   public webrtc::DataChannelObserver

{
  public:
    LLWebRTCPeerConnectionImpl() {}
    ~LLWebRTCPeerConnectionImpl() {}

    void init(LLWebRTCImpl * webrtc_impl);
    void terminate();

    virtual void AddRef() const override = 0;
    virtual rtc::RefCountReleaseStatus Release() const override = 0;
    
    //
    // LLWebRTCPeerConnection
    //

    void setSignalingObserver(LLWebRTCSignalingObserver *observer) override;
    void unsetSignalingObserver(LLWebRTCSignalingObserver *observer) override;
    bool initializeConnection() override;
    void shutdownConnection() override;
    void AnswerAvailable(const std::string &sdp) override;


    //
    // LLWebRTCAudioInterface
    //
    void setMute(bool mute) override;
    
    //
    // LLWebRTCDataInterface
    //
    void sendData(const std::string& data, bool binary=false) override;
    void setDataObserver(LLWebRTCDataObserver *observer) override;
    void unsetDataObserver(LLWebRTCDataObserver *observer) override;

    //
    // PeerConnectionObserver implementation.
    //

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>                     receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;
    void OnIceConnectionReceivingChange(bool receiving) override {}
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;

    //
    // CreateSessionDescriptionObserver implementation.
    //
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;
    void OnFailure(webrtc::RTCError error) override;

    //
    // SetRemoteDescriptionObserverInterface implementation.
    //
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

    //
    // SetLocalDescriptionObserverInterface implementation.
    //
    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;
    
    //
    // DataChannelObserver implementation.
    //
    void OnStateChange() override;
    void OnMessage(const webrtc::DataBuffer& buffer) override;
    
    // Helpers
    void enableTracks(bool enable);

  protected:
    
    LLWebRTCImpl * mWebRTCImpl;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;

    bool                                                       mMute;

    // signaling
    std::vector<LLWebRTCSignalingObserver *>                   mSignalingObserverList;
    std::vector<std::unique_ptr<webrtc::IceCandidateInterface>>  mCachedIceCandidates;
    bool                                                       mAnswerReceived;

    rtc::scoped_refptr<webrtc::PeerConnectionInterface>        mPeerConnection;

    std::vector<LLWebRTCDataObserver *>                        mDataObserverList;
    rtc::scoped_refptr<webrtc::DataChannelInterface>           mDataChannel;
};
        
}

#endif // LLWEBRTC_IMPL_H
