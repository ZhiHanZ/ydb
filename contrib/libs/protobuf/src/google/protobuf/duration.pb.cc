// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: google/protobuf/duration.proto

#include <google/protobuf/duration.pb.h>

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
PROTOBUF_NAMESPACE_OPEN
constexpr Duration::Duration(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : seconds_(int64_t{0})
  , nanos_(0){}
struct DurationDefaultTypeInternal {
  constexpr DurationDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~DurationDefaultTypeInternal() {}
  union {
    Duration _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT DurationDefaultTypeInternal _Duration_default_instance_;
PROTOBUF_NAMESPACE_CLOSE
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_google_2fprotobuf_2fduration_2eproto[1];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_google_2fprotobuf_2fduration_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_google_2fprotobuf_2fduration_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_google_2fprotobuf_2fduration_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(PROTOBUF_NAMESPACE_ID::Duration, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(PROTOBUF_NAMESPACE_ID::Duration, seconds_),
  PROTOBUF_FIELD_OFFSET(PROTOBUF_NAMESPACE_ID::Duration, nanos_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(PROTOBUF_NAMESPACE_ID::Duration)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&PROTOBUF_NAMESPACE_ID::_Duration_default_instance_),
};

const char descriptor_table_protodef_google_2fprotobuf_2fduration_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\036google/protobuf/duration.proto\022\017google"
  ".protobuf\"*\n\010Duration\022\017\n\007seconds\030\001 \001(\003\022\r"
  "\n\005nanos\030\002 \001(\005B\203\001\n\023com.google.protobufB\rD"
  "urationProtoP\001Z1google.golang.org/protob"
  "uf/types/known/durationpb\370\001\001\242\002\003GPB\252\002\036Goo"
  "gle.Protobuf.WellKnownTypesb\006proto3"
  ;
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_google_2fprotobuf_2fduration_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_google_2fprotobuf_2fduration_2eproto = {
  false, false, 235, descriptor_table_protodef_google_2fprotobuf_2fduration_2eproto, "google/protobuf/duration.proto", 
  &descriptor_table_google_2fprotobuf_2fduration_2eproto_once, nullptr, 0, 1,
  schemas, file_default_instances, TableStruct_google_2fprotobuf_2fduration_2eproto::offsets,
  file_level_metadata_google_2fprotobuf_2fduration_2eproto, file_level_enum_descriptors_google_2fprotobuf_2fduration_2eproto, file_level_service_descriptors_google_2fprotobuf_2fduration_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable* descriptor_table_google_2fprotobuf_2fduration_2eproto_getter() {
  return &descriptor_table_google_2fprotobuf_2fduration_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_google_2fprotobuf_2fduration_2eproto(&descriptor_table_google_2fprotobuf_2fduration_2eproto);
PROTOBUF_NAMESPACE_OPEN

// ===================================================================

class Duration::_Internal {
 public:
};

Duration::Duration(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:google.protobuf.Duration)
}
Duration::Duration(const Duration& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&seconds_, &from.seconds_,
    static_cast<size_t>(reinterpret_cast<char*>(&nanos_) -
    reinterpret_cast<char*>(&seconds_)) + sizeof(nanos_));
  // @@protoc_insertion_point(copy_constructor:google.protobuf.Duration)
}

void Duration::SharedCtor() {
::memset(reinterpret_cast<char*>(this) + static_cast<size_t>(
    reinterpret_cast<char*>(&seconds_) - reinterpret_cast<char*>(this)),
    0, static_cast<size_t>(reinterpret_cast<char*>(&nanos_) -
    reinterpret_cast<char*>(&seconds_)) + sizeof(nanos_));
}

Duration::~Duration() {
  // @@protoc_insertion_point(destructor:google.protobuf.Duration)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void Duration::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void Duration::ArenaDtor(void* object) {
  Duration* _this = reinterpret_cast< Duration* >(object);
  (void)_this;
}
void Duration::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void Duration::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void Duration::Clear() {
// @@protoc_insertion_point(message_clear_start:google.protobuf.Duration)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&seconds_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&nanos_) -
      reinterpret_cast<char*>(&seconds_)) + sizeof(nanos_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* Duration::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // int64 seconds = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          seconds_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // int32 nanos = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 16)) {
          nanos_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag == 0) || ((tag & 7) == 4)) {
          CHK_(ptr);
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* Duration::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:google.protobuf.Duration)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // int64 seconds = 1;
  if (this->_internal_seconds() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt64ToArray(1, this->_internal_seconds(), target);
  }

  // int32 nanos = 2;
  if (this->_internal_nanos() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(2, this->_internal_nanos(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:google.protobuf.Duration)
  return target;
}

size_t Duration::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:google.protobuf.Duration)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int64 seconds = 1;
  if (this->_internal_seconds() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int64Size(
        this->_internal_seconds());
  }

  // int32 nanos = 2;
  if (this->_internal_nanos() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_nanos());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData Duration::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    Duration::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*Duration::GetClassData() const { return &_class_data_; }

void Duration::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to,
                      const ::PROTOBUF_NAMESPACE_ID::Message&from) {
  static_cast<Duration *>(to)->MergeFrom(
      static_cast<const Duration &>(from));
}


void Duration::MergeFrom(const Duration& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:google.protobuf.Duration)
  GOOGLE_DCHECK_NE(&from, this);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_seconds() != 0) {
    _internal_set_seconds(from._internal_seconds());
  }
  if (from._internal_nanos() != 0) {
    _internal_set_nanos(from._internal_nanos());
  }
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void Duration::CopyFrom(const Duration& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:google.protobuf.Duration)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Duration::IsInitialized() const {
  return true;
}

void Duration::InternalSwap(Duration* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Duration, nanos_)
      + sizeof(Duration::nanos_)
      - PROTOBUF_FIELD_OFFSET(Duration, seconds_)>(
          reinterpret_cast<char*>(&seconds_),
          reinterpret_cast<char*>(&other->seconds_));
}

::PROTOBUF_NAMESPACE_ID::Metadata Duration::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_google_2fprotobuf_2fduration_2eproto_getter, &descriptor_table_google_2fprotobuf_2fduration_2eproto_once,
      file_level_metadata_google_2fprotobuf_2fduration_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
PROTOBUF_NAMESPACE_CLOSE
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE PROTOBUF_NAMESPACE_ID::Duration* Arena::CreateMaybeMessage< PROTOBUF_NAMESPACE_ID::Duration >(Arena* arena) {
  return Arena::CreateMessageInternal< PROTOBUF_NAMESPACE_ID::Duration >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
