// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: player_command.proto

#ifndef PROTOBUF_player_5fcommand_2eproto__INCLUDED
#define PROTOBUF_player_5fcommand_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_player_5fcommand_2eproto();
void protobuf_AssignDesc_player_5fcommand_2eproto();
void protobuf_ShutdownFile_player_5fcommand_2eproto();

class RequestIdentifier;
class PlayerCommandMsg;
class PlayerCommandMsg_NewSongRequest;
class PlayerCommandReplyMsg;

// ===================================================================

class RequestIdentifier : public ::google::protobuf::Message {
 public:
  RequestIdentifier();
  virtual ~RequestIdentifier();

  RequestIdentifier(const RequestIdentifier& from);

  inline RequestIdentifier& operator=(const RequestIdentifier& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const RequestIdentifier& default_instance();

  void Swap(RequestIdentifier* other);

  // implements Message ----------------------------------------------

  RequestIdentifier* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const RequestIdentifier& from);
  void MergeFrom(const RequestIdentifier& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required uint64 cookie = 1;
  inline bool has_cookie() const;
  inline void clear_cookie();
  static const int kCookieFieldNumber = 1;
  inline ::google::protobuf::uint64 cookie() const;
  inline void set_cookie(::google::protobuf::uint64 value);

  // required uint64 requestor_guid = 2;
  inline bool has_requestor_guid() const;
  inline void clear_requestor_guid();
  static const int kRequestorGuidFieldNumber = 2;
  inline ::google::protobuf::uint64 requestor_guid() const;
  inline void set_requestor_guid(::google::protobuf::uint64 value);

  // required string requestor_name = 3;
  inline bool has_requestor_name() const;
  inline void clear_requestor_name();
  static const int kRequestorNameFieldNumber = 3;
  inline const ::std::string& requestor_name() const;
  inline void set_requestor_name(const ::std::string& value);
  inline void set_requestor_name(const char* value);
  inline void set_requestor_name(const char* value, size_t size);
  inline ::std::string* mutable_requestor_name();
  inline ::std::string* release_requestor_name();
  inline void set_allocated_requestor_name(::std::string* requestor_name);

  // @@protoc_insertion_point(class_scope:RequestIdentifier)
 private:
  inline void set_has_cookie();
  inline void clear_has_cookie();
  inline void set_has_requestor_guid();
  inline void clear_has_requestor_guid();
  inline void set_has_requestor_name();
  inline void clear_has_requestor_name();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::uint64 cookie_;
  ::google::protobuf::uint64 requestor_guid_;
  ::std::string* requestor_name_;
  friend void  protobuf_AddDesc_player_5fcommand_2eproto();
  friend void protobuf_AssignDesc_player_5fcommand_2eproto();
  friend void protobuf_ShutdownFile_player_5fcommand_2eproto();

  void InitAsDefaultInstance();
  static RequestIdentifier* default_instance_;
};
// -------------------------------------------------------------------

class PlayerCommandMsg_NewSongRequest : public ::google::protobuf::Message {
 public:
  PlayerCommandMsg_NewSongRequest();
  virtual ~PlayerCommandMsg_NewSongRequest();

  PlayerCommandMsg_NewSongRequest(const PlayerCommandMsg_NewSongRequest& from);

  inline PlayerCommandMsg_NewSongRequest& operator=(const PlayerCommandMsg_NewSongRequest& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const PlayerCommandMsg_NewSongRequest& default_instance();

  void Swap(PlayerCommandMsg_NewSongRequest* other);

  // implements Message ----------------------------------------------

  PlayerCommandMsg_NewSongRequest* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PlayerCommandMsg_NewSongRequest& from);
  void MergeFrom(const PlayerCommandMsg_NewSongRequest& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required string song_name = 1;
  inline bool has_song_name() const;
  inline void clear_song_name();
  static const int kSongNameFieldNumber = 1;
  inline const ::std::string& song_name() const;
  inline void set_song_name(const ::std::string& value);
  inline void set_song_name(const char* value);
  inline void set_song_name(const char* value, size_t size);
  inline ::std::string* mutable_song_name();
  inline ::std::string* release_song_name();
  inline void set_allocated_song_name(::std::string* song_name);

  // optional uint32 position_in_ms = 2 [default = 0];
  inline bool has_position_in_ms() const;
  inline void clear_position_in_ms();
  static const int kPositionInMsFieldNumber = 2;
  inline ::google::protobuf::uint32 position_in_ms() const;
  inline void set_position_in_ms(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:PlayerCommandMsg.NewSongRequest)
 private:
  inline void set_has_song_name();
  inline void clear_has_song_name();
  inline void set_has_position_in_ms();
  inline void clear_has_position_in_ms();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* song_name_;
  ::google::protobuf::uint32 position_in_ms_;
  friend void  protobuf_AddDesc_player_5fcommand_2eproto();
  friend void protobuf_AssignDesc_player_5fcommand_2eproto();
  friend void protobuf_ShutdownFile_player_5fcommand_2eproto();

  void InitAsDefaultInstance();
  static PlayerCommandMsg_NewSongRequest* default_instance_;
};
// -------------------------------------------------------------------

class PlayerCommandMsg : public ::google::protobuf::Message {
 public:
  PlayerCommandMsg();
  virtual ~PlayerCommandMsg();

  PlayerCommandMsg(const PlayerCommandMsg& from);

  inline PlayerCommandMsg& operator=(const PlayerCommandMsg& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const PlayerCommandMsg& default_instance();

  void Swap(PlayerCommandMsg* other);

  // implements Message ----------------------------------------------

  PlayerCommandMsg* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PlayerCommandMsg& from);
  void MergeFrom(const PlayerCommandMsg& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  typedef PlayerCommandMsg_NewSongRequest NewSongRequest;

  // accessors -------------------------------------------------------

  // required .RequestIdentifier req_identifier = 1;
  inline bool has_req_identifier() const;
  inline void clear_req_identifier();
  static const int kReqIdentifierFieldNumber = 1;
  inline const ::RequestIdentifier& req_identifier() const;
  inline ::RequestIdentifier* mutable_req_identifier();
  inline ::RequestIdentifier* release_req_identifier();
  inline void set_allocated_req_identifier(::RequestIdentifier* req_identifier);

  // optional bool stop_play = 2;
  inline bool has_stop_play() const;
  inline void clear_stop_play();
  static const int kStopPlayFieldNumber = 2;
  inline bool stop_play() const;
  inline void set_stop_play(bool value);

  // optional .PlayerCommandMsg.NewSongRequest new_song_request = 3;
  inline bool has_new_song_request() const;
  inline void clear_new_song_request();
  static const int kNewSongRequestFieldNumber = 3;
  inline const ::PlayerCommandMsg_NewSongRequest& new_song_request() const;
  inline ::PlayerCommandMsg_NewSongRequest* mutable_new_song_request();
  inline ::PlayerCommandMsg_NewSongRequest* release_new_song_request();
  inline void set_allocated_new_song_request(::PlayerCommandMsg_NewSongRequest* new_song_request);

  // @@protoc_insertion_point(class_scope:PlayerCommandMsg)
 private:
  inline void set_has_req_identifier();
  inline void clear_has_req_identifier();
  inline void set_has_stop_play();
  inline void clear_has_stop_play();
  inline void set_has_new_song_request();
  inline void clear_has_new_song_request();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::RequestIdentifier* req_identifier_;
  ::PlayerCommandMsg_NewSongRequest* new_song_request_;
  bool stop_play_;
  friend void  protobuf_AddDesc_player_5fcommand_2eproto();
  friend void protobuf_AssignDesc_player_5fcommand_2eproto();
  friend void protobuf_ShutdownFile_player_5fcommand_2eproto();

  void InitAsDefaultInstance();
  static PlayerCommandMsg* default_instance_;
};
// -------------------------------------------------------------------

class PlayerCommandReplyMsg : public ::google::protobuf::Message {
 public:
  PlayerCommandReplyMsg();
  virtual ~PlayerCommandReplyMsg();

  PlayerCommandReplyMsg(const PlayerCommandReplyMsg& from);

  inline PlayerCommandReplyMsg& operator=(const PlayerCommandReplyMsg& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const PlayerCommandReplyMsg& default_instance();

  void Swap(PlayerCommandReplyMsg* other);

  // implements Message ----------------------------------------------

  PlayerCommandReplyMsg* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PlayerCommandReplyMsg& from);
  void MergeFrom(const PlayerCommandReplyMsg& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required .RequestIdentifier req_identifier = 1;
  inline bool has_req_identifier() const;
  inline void clear_req_identifier();
  static const int kReqIdentifierFieldNumber = 1;
  inline const ::RequestIdentifier& req_identifier() const;
  inline ::RequestIdentifier* mutable_req_identifier();
  inline ::RequestIdentifier* release_req_identifier();
  inline void set_allocated_req_identifier(::RequestIdentifier* req_identifier);

  // required bool req_status = 2;
  inline bool has_req_status() const;
  inline void clear_req_status();
  static const int kReqStatusFieldNumber = 2;
  inline bool req_status() const;
  inline void set_req_status(bool value);

  // required string req_status_desc = 3;
  inline bool has_req_status_desc() const;
  inline void clear_req_status_desc();
  static const int kReqStatusDescFieldNumber = 3;
  inline const ::std::string& req_status_desc() const;
  inline void set_req_status_desc(const ::std::string& value);
  inline void set_req_status_desc(const char* value);
  inline void set_req_status_desc(const char* value, size_t size);
  inline ::std::string* mutable_req_status_desc();
  inline ::std::string* release_req_status_desc();
  inline void set_allocated_req_status_desc(::std::string* req_status_desc);

  // @@protoc_insertion_point(class_scope:PlayerCommandReplyMsg)
 private:
  inline void set_has_req_identifier();
  inline void clear_has_req_identifier();
  inline void set_has_req_status();
  inline void clear_has_req_status();
  inline void set_has_req_status_desc();
  inline void clear_has_req_status_desc();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::RequestIdentifier* req_identifier_;
  ::std::string* req_status_desc_;
  bool req_status_;
  friend void  protobuf_AddDesc_player_5fcommand_2eproto();
  friend void protobuf_AssignDesc_player_5fcommand_2eproto();
  friend void protobuf_ShutdownFile_player_5fcommand_2eproto();

  void InitAsDefaultInstance();
  static PlayerCommandReplyMsg* default_instance_;
};
// ===================================================================


// ===================================================================

// RequestIdentifier

// required uint64 cookie = 1;
inline bool RequestIdentifier::has_cookie() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void RequestIdentifier::set_has_cookie() {
  _has_bits_[0] |= 0x00000001u;
}
inline void RequestIdentifier::clear_has_cookie() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void RequestIdentifier::clear_cookie() {
  cookie_ = GOOGLE_ULONGLONG(0);
  clear_has_cookie();
}
inline ::google::protobuf::uint64 RequestIdentifier::cookie() const {
  // @@protoc_insertion_point(field_get:RequestIdentifier.cookie)
  return cookie_;
}
inline void RequestIdentifier::set_cookie(::google::protobuf::uint64 value) {
  set_has_cookie();
  cookie_ = value;
  // @@protoc_insertion_point(field_set:RequestIdentifier.cookie)
}

// required uint64 requestor_guid = 2;
inline bool RequestIdentifier::has_requestor_guid() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void RequestIdentifier::set_has_requestor_guid() {
  _has_bits_[0] |= 0x00000002u;
}
inline void RequestIdentifier::clear_has_requestor_guid() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void RequestIdentifier::clear_requestor_guid() {
  requestor_guid_ = GOOGLE_ULONGLONG(0);
  clear_has_requestor_guid();
}
inline ::google::protobuf::uint64 RequestIdentifier::requestor_guid() const {
  // @@protoc_insertion_point(field_get:RequestIdentifier.requestor_guid)
  return requestor_guid_;
}
inline void RequestIdentifier::set_requestor_guid(::google::protobuf::uint64 value) {
  set_has_requestor_guid();
  requestor_guid_ = value;
  // @@protoc_insertion_point(field_set:RequestIdentifier.requestor_guid)
}

// required string requestor_name = 3;
inline bool RequestIdentifier::has_requestor_name() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void RequestIdentifier::set_has_requestor_name() {
  _has_bits_[0] |= 0x00000004u;
}
inline void RequestIdentifier::clear_has_requestor_name() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void RequestIdentifier::clear_requestor_name() {
  if (requestor_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    requestor_name_->clear();
  }
  clear_has_requestor_name();
}
inline const ::std::string& RequestIdentifier::requestor_name() const {
  // @@protoc_insertion_point(field_get:RequestIdentifier.requestor_name)
  return *requestor_name_;
}
inline void RequestIdentifier::set_requestor_name(const ::std::string& value) {
  set_has_requestor_name();
  if (requestor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    requestor_name_ = new ::std::string;
  }
  requestor_name_->assign(value);
  // @@protoc_insertion_point(field_set:RequestIdentifier.requestor_name)
}
inline void RequestIdentifier::set_requestor_name(const char* value) {
  set_has_requestor_name();
  if (requestor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    requestor_name_ = new ::std::string;
  }
  requestor_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:RequestIdentifier.requestor_name)
}
inline void RequestIdentifier::set_requestor_name(const char* value, size_t size) {
  set_has_requestor_name();
  if (requestor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    requestor_name_ = new ::std::string;
  }
  requestor_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:RequestIdentifier.requestor_name)
}
inline ::std::string* RequestIdentifier::mutable_requestor_name() {
  set_has_requestor_name();
  if (requestor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    requestor_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:RequestIdentifier.requestor_name)
  return requestor_name_;
}
inline ::std::string* RequestIdentifier::release_requestor_name() {
  clear_has_requestor_name();
  if (requestor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = requestor_name_;
    requestor_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void RequestIdentifier::set_allocated_requestor_name(::std::string* requestor_name) {
  if (requestor_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete requestor_name_;
  }
  if (requestor_name) {
    set_has_requestor_name();
    requestor_name_ = requestor_name;
  } else {
    clear_has_requestor_name();
    requestor_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:RequestIdentifier.requestor_name)
}

// -------------------------------------------------------------------

// PlayerCommandMsg_NewSongRequest

// required string song_name = 1;
inline bool PlayerCommandMsg_NewSongRequest::has_song_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PlayerCommandMsg_NewSongRequest::set_has_song_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PlayerCommandMsg_NewSongRequest::clear_has_song_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PlayerCommandMsg_NewSongRequest::clear_song_name() {
  if (song_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    song_name_->clear();
  }
  clear_has_song_name();
}
inline const ::std::string& PlayerCommandMsg_NewSongRequest::song_name() const {
  // @@protoc_insertion_point(field_get:PlayerCommandMsg.NewSongRequest.song_name)
  return *song_name_;
}
inline void PlayerCommandMsg_NewSongRequest::set_song_name(const ::std::string& value) {
  set_has_song_name();
  if (song_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    song_name_ = new ::std::string;
  }
  song_name_->assign(value);
  // @@protoc_insertion_point(field_set:PlayerCommandMsg.NewSongRequest.song_name)
}
inline void PlayerCommandMsg_NewSongRequest::set_song_name(const char* value) {
  set_has_song_name();
  if (song_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    song_name_ = new ::std::string;
  }
  song_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:PlayerCommandMsg.NewSongRequest.song_name)
}
inline void PlayerCommandMsg_NewSongRequest::set_song_name(const char* value, size_t size) {
  set_has_song_name();
  if (song_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    song_name_ = new ::std::string;
  }
  song_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:PlayerCommandMsg.NewSongRequest.song_name)
}
inline ::std::string* PlayerCommandMsg_NewSongRequest::mutable_song_name() {
  set_has_song_name();
  if (song_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    song_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:PlayerCommandMsg.NewSongRequest.song_name)
  return song_name_;
}
inline ::std::string* PlayerCommandMsg_NewSongRequest::release_song_name() {
  clear_has_song_name();
  if (song_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = song_name_;
    song_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void PlayerCommandMsg_NewSongRequest::set_allocated_song_name(::std::string* song_name) {
  if (song_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete song_name_;
  }
  if (song_name) {
    set_has_song_name();
    song_name_ = song_name;
  } else {
    clear_has_song_name();
    song_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:PlayerCommandMsg.NewSongRequest.song_name)
}

// optional uint32 position_in_ms = 2 [default = 0];
inline bool PlayerCommandMsg_NewSongRequest::has_position_in_ms() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PlayerCommandMsg_NewSongRequest::set_has_position_in_ms() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PlayerCommandMsg_NewSongRequest::clear_has_position_in_ms() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PlayerCommandMsg_NewSongRequest::clear_position_in_ms() {
  position_in_ms_ = 0u;
  clear_has_position_in_ms();
}
inline ::google::protobuf::uint32 PlayerCommandMsg_NewSongRequest::position_in_ms() const {
  // @@protoc_insertion_point(field_get:PlayerCommandMsg.NewSongRequest.position_in_ms)
  return position_in_ms_;
}
inline void PlayerCommandMsg_NewSongRequest::set_position_in_ms(::google::protobuf::uint32 value) {
  set_has_position_in_ms();
  position_in_ms_ = value;
  // @@protoc_insertion_point(field_set:PlayerCommandMsg.NewSongRequest.position_in_ms)
}

// -------------------------------------------------------------------

// PlayerCommandMsg

// required .RequestIdentifier req_identifier = 1;
inline bool PlayerCommandMsg::has_req_identifier() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PlayerCommandMsg::set_has_req_identifier() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PlayerCommandMsg::clear_has_req_identifier() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PlayerCommandMsg::clear_req_identifier() {
  if (req_identifier_ != NULL) req_identifier_->::RequestIdentifier::Clear();
  clear_has_req_identifier();
}
inline const ::RequestIdentifier& PlayerCommandMsg::req_identifier() const {
  // @@protoc_insertion_point(field_get:PlayerCommandMsg.req_identifier)
  return req_identifier_ != NULL ? *req_identifier_ : *default_instance_->req_identifier_;
}
inline ::RequestIdentifier* PlayerCommandMsg::mutable_req_identifier() {
  set_has_req_identifier();
  if (req_identifier_ == NULL) req_identifier_ = new ::RequestIdentifier;
  // @@protoc_insertion_point(field_mutable:PlayerCommandMsg.req_identifier)
  return req_identifier_;
}
inline ::RequestIdentifier* PlayerCommandMsg::release_req_identifier() {
  clear_has_req_identifier();
  ::RequestIdentifier* temp = req_identifier_;
  req_identifier_ = NULL;
  return temp;
}
inline void PlayerCommandMsg::set_allocated_req_identifier(::RequestIdentifier* req_identifier) {
  delete req_identifier_;
  req_identifier_ = req_identifier;
  if (req_identifier) {
    set_has_req_identifier();
  } else {
    clear_has_req_identifier();
  }
  // @@protoc_insertion_point(field_set_allocated:PlayerCommandMsg.req_identifier)
}

// optional bool stop_play = 2;
inline bool PlayerCommandMsg::has_stop_play() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PlayerCommandMsg::set_has_stop_play() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PlayerCommandMsg::clear_has_stop_play() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PlayerCommandMsg::clear_stop_play() {
  stop_play_ = false;
  clear_has_stop_play();
}
inline bool PlayerCommandMsg::stop_play() const {
  // @@protoc_insertion_point(field_get:PlayerCommandMsg.stop_play)
  return stop_play_;
}
inline void PlayerCommandMsg::set_stop_play(bool value) {
  set_has_stop_play();
  stop_play_ = value;
  // @@protoc_insertion_point(field_set:PlayerCommandMsg.stop_play)
}

// optional .PlayerCommandMsg.NewSongRequest new_song_request = 3;
inline bool PlayerCommandMsg::has_new_song_request() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void PlayerCommandMsg::set_has_new_song_request() {
  _has_bits_[0] |= 0x00000004u;
}
inline void PlayerCommandMsg::clear_has_new_song_request() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void PlayerCommandMsg::clear_new_song_request() {
  if (new_song_request_ != NULL) new_song_request_->::PlayerCommandMsg_NewSongRequest::Clear();
  clear_has_new_song_request();
}
inline const ::PlayerCommandMsg_NewSongRequest& PlayerCommandMsg::new_song_request() const {
  // @@protoc_insertion_point(field_get:PlayerCommandMsg.new_song_request)
  return new_song_request_ != NULL ? *new_song_request_ : *default_instance_->new_song_request_;
}
inline ::PlayerCommandMsg_NewSongRequest* PlayerCommandMsg::mutable_new_song_request() {
  set_has_new_song_request();
  if (new_song_request_ == NULL) new_song_request_ = new ::PlayerCommandMsg_NewSongRequest;
  // @@protoc_insertion_point(field_mutable:PlayerCommandMsg.new_song_request)
  return new_song_request_;
}
inline ::PlayerCommandMsg_NewSongRequest* PlayerCommandMsg::release_new_song_request() {
  clear_has_new_song_request();
  ::PlayerCommandMsg_NewSongRequest* temp = new_song_request_;
  new_song_request_ = NULL;
  return temp;
}
inline void PlayerCommandMsg::set_allocated_new_song_request(::PlayerCommandMsg_NewSongRequest* new_song_request) {
  delete new_song_request_;
  new_song_request_ = new_song_request;
  if (new_song_request) {
    set_has_new_song_request();
  } else {
    clear_has_new_song_request();
  }
  // @@protoc_insertion_point(field_set_allocated:PlayerCommandMsg.new_song_request)
}

// -------------------------------------------------------------------

// PlayerCommandReplyMsg

// required .RequestIdentifier req_identifier = 1;
inline bool PlayerCommandReplyMsg::has_req_identifier() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PlayerCommandReplyMsg::set_has_req_identifier() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PlayerCommandReplyMsg::clear_has_req_identifier() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PlayerCommandReplyMsg::clear_req_identifier() {
  if (req_identifier_ != NULL) req_identifier_->::RequestIdentifier::Clear();
  clear_has_req_identifier();
}
inline const ::RequestIdentifier& PlayerCommandReplyMsg::req_identifier() const {
  // @@protoc_insertion_point(field_get:PlayerCommandReplyMsg.req_identifier)
  return req_identifier_ != NULL ? *req_identifier_ : *default_instance_->req_identifier_;
}
inline ::RequestIdentifier* PlayerCommandReplyMsg::mutable_req_identifier() {
  set_has_req_identifier();
  if (req_identifier_ == NULL) req_identifier_ = new ::RequestIdentifier;
  // @@protoc_insertion_point(field_mutable:PlayerCommandReplyMsg.req_identifier)
  return req_identifier_;
}
inline ::RequestIdentifier* PlayerCommandReplyMsg::release_req_identifier() {
  clear_has_req_identifier();
  ::RequestIdentifier* temp = req_identifier_;
  req_identifier_ = NULL;
  return temp;
}
inline void PlayerCommandReplyMsg::set_allocated_req_identifier(::RequestIdentifier* req_identifier) {
  delete req_identifier_;
  req_identifier_ = req_identifier;
  if (req_identifier) {
    set_has_req_identifier();
  } else {
    clear_has_req_identifier();
  }
  // @@protoc_insertion_point(field_set_allocated:PlayerCommandReplyMsg.req_identifier)
}

// required bool req_status = 2;
inline bool PlayerCommandReplyMsg::has_req_status() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PlayerCommandReplyMsg::set_has_req_status() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PlayerCommandReplyMsg::clear_has_req_status() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PlayerCommandReplyMsg::clear_req_status() {
  req_status_ = false;
  clear_has_req_status();
}
inline bool PlayerCommandReplyMsg::req_status() const {
  // @@protoc_insertion_point(field_get:PlayerCommandReplyMsg.req_status)
  return req_status_;
}
inline void PlayerCommandReplyMsg::set_req_status(bool value) {
  set_has_req_status();
  req_status_ = value;
  // @@protoc_insertion_point(field_set:PlayerCommandReplyMsg.req_status)
}

// required string req_status_desc = 3;
inline bool PlayerCommandReplyMsg::has_req_status_desc() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void PlayerCommandReplyMsg::set_has_req_status_desc() {
  _has_bits_[0] |= 0x00000004u;
}
inline void PlayerCommandReplyMsg::clear_has_req_status_desc() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void PlayerCommandReplyMsg::clear_req_status_desc() {
  if (req_status_desc_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    req_status_desc_->clear();
  }
  clear_has_req_status_desc();
}
inline const ::std::string& PlayerCommandReplyMsg::req_status_desc() const {
  // @@protoc_insertion_point(field_get:PlayerCommandReplyMsg.req_status_desc)
  return *req_status_desc_;
}
inline void PlayerCommandReplyMsg::set_req_status_desc(const ::std::string& value) {
  set_has_req_status_desc();
  if (req_status_desc_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    req_status_desc_ = new ::std::string;
  }
  req_status_desc_->assign(value);
  // @@protoc_insertion_point(field_set:PlayerCommandReplyMsg.req_status_desc)
}
inline void PlayerCommandReplyMsg::set_req_status_desc(const char* value) {
  set_has_req_status_desc();
  if (req_status_desc_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    req_status_desc_ = new ::std::string;
  }
  req_status_desc_->assign(value);
  // @@protoc_insertion_point(field_set_char:PlayerCommandReplyMsg.req_status_desc)
}
inline void PlayerCommandReplyMsg::set_req_status_desc(const char* value, size_t size) {
  set_has_req_status_desc();
  if (req_status_desc_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    req_status_desc_ = new ::std::string;
  }
  req_status_desc_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:PlayerCommandReplyMsg.req_status_desc)
}
inline ::std::string* PlayerCommandReplyMsg::mutable_req_status_desc() {
  set_has_req_status_desc();
  if (req_status_desc_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    req_status_desc_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:PlayerCommandReplyMsg.req_status_desc)
  return req_status_desc_;
}
inline ::std::string* PlayerCommandReplyMsg::release_req_status_desc() {
  clear_has_req_status_desc();
  if (req_status_desc_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = req_status_desc_;
    req_status_desc_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void PlayerCommandReplyMsg::set_allocated_req_status_desc(::std::string* req_status_desc) {
  if (req_status_desc_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete req_status_desc_;
  }
  if (req_status_desc) {
    set_has_req_status_desc();
    req_status_desc_ = req_status_desc;
  } else {
    clear_has_req_status_desc();
    req_status_desc_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:PlayerCommandReplyMsg.req_status_desc)
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_player_5fcommand_2eproto__INCLUDED