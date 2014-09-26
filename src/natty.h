 /**
 * Copyright (C) 2014 Lantern
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef NATTY_H_
#define NATTY_H_
#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>

#include "webrtc/base/socketaddress.h"
#include "talk/app/webrtc/portallocatorfactory.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnection.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "talk/app/webrtc/datachannelinterface.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/json.h"

class NattyDataChannelObserver 
: public webrtc::DataChannelObserver {
 public:
  explicit NattyDataChannelObserver(webrtc::DataChannelInterface* channel)
      : channel_(channel), received_message_count_(0) {
        channel_->RegisterObserver(this);
        state_ = channel_->state();
      }
  virtual ~NattyDataChannelObserver() {
    channel_->UnregisterObserver();
  }
  virtual void OnStateChange() { 
    LOG(INFO) << "Data channel state changed";
    state_ = channel_->state(); 
  }
  virtual void OnMessage(const webrtc::DataBuffer& buffer) {
    LOG(INFO) << "Received data buffer message";
    last_message_.assign(buffer.data.data(), buffer.data.length());
    ++received_message_count_;
  }

  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  const std::string& last_message() const { return last_message_; }
  size_t received_message_count() const { return received_message_count_; }

 private:
  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  webrtc::DataChannelInterface::DataState state_;
  std::string last_message_;
  size_t received_message_count_;
};


class NattySessionObserver
: public webrtc::SetSessionDescriptionObserver {
 public:
  static NattySessionObserver* Create() {
    return
        new rtc::RefCountedObject<NattySessionObserver>();
  }
  virtual void OnSuccess() {
    LOG(INFO) << __FUNCTION__;
  }
  virtual void OnFailure(const std::string& error) {
    LOG(INFO) << __FUNCTION__ << " " << error;
  }

 protected:
  NattySessionObserver() {}
  ~NattySessionObserver() {}
};


class Natty
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver {
 public:
  Natty(rtc::Thread* thread);

  bool connection_active() const;

  virtual void Init(bool mode);
  virtual void OpenDumpFile(const std::string& filename);
  bool InitializePeerConnection();
  void Shutdown();
  void ProcessInput();
  virtual void ReadMessage(const std::string& message);
  
 protected:
  ~Natty();
 
  void AddStreams(); 

  //
  // PeerConnectionObserver implementation.
  //
  virtual void InspectTransportChannel();
  virtual void OnError();
  virtual void OnStateChange(
      webrtc::PeerConnectionObserver::StateType state_changed);
  virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state);

    virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
  virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
  virtual void OnRenegotiationNeeded();
  virtual void OnIceConnectionChange(webrtc::PeerConnection::IceConnectionState new_state);
  virtual void OnIceComplete();

  virtual void OnDataChannel(webrtc::DataChannelInterface *data_channel);
  virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

  virtual void PickFinalCandidate();
  virtual void OnFailure(const std::string& msg);

  virtual void Output5Tuple(const cricket::Candidate *cand);


  virtual void OnServerConnectionFailure();

  // CreateSessionDescriptionObserver implementation.
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
 
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;  
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
  NattyDataChannelObserver* data_channel_observer_;

 protected:
  int peer_id_;
  rtc::Thread* thread_;
  rtc::scoped_refptr<webrtc::MediaStreamInterface> stream;

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  std::deque<std::string*> pending_messages_;



  webrtc::SessionDescriptionInterface* session_description;
  int sdp_mlineindex;
  rtc::scoped_refptr<webrtc::PortAllocatorFactoryInterface> allocator_factory_;
  cricket::PortAllocator* allocator;

  uint32 highestPrioritySeen;
  const cricket::Candidate *bestCandidate;

  /* stdout */
  std::ofstream outfile;

  enum Mode { OFFER, TRAVERSE };

  void setMode(Mode m);
  //Mode getMode() const;
  Mode mode;

  Json::Value fivetuple;

  std::map<std::string, rtc::scoped_refptr<webrtc::MediaStreamInterface> >
    active_streams_;


  class InputStream {
    public:
      InputStream() {}
      ~InputStream() {}

      void read(Natty* natty);

      std::string build() const;

    protected:
      std::stringstream ss;
      std::string input;
  };    


};

#endif  // NATTY_H_
