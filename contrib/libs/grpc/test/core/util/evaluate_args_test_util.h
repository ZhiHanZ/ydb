// Copyright 2021 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GRPC_TEST_CORE_UTIL_EVALUATE_ARGS_TEST_UTIL_H
#define GRPC_TEST_CORE_UTIL_EVALUATE_ARGS_TEST_UTIL_H

#include <grpc/support/port_platform.h>

#include <list>

#include <gtest/gtest.h>

#include "src/core/lib/security/authorization/evaluate_args.h"
#include "test/core/util/mock_authorization_endpoint.h"

namespace grpc_core {

class EvaluateArgsTestUtil {
 public:
  EvaluateArgsTestUtil() = default;

  ~EvaluateArgsTestUtil() { delete channel_args_; }

  void AddPairToMetadata(const char* key, const char* value) {
    metadata_.Append(
        key, Slice(grpc_slice_intern(grpc_slice_from_static_string(value))));
  }

  void SetLocalEndpoint(y_absl::string_view local_uri) {
    endpoint_.SetLocalAddress(local_uri);
  }

  void SetPeerEndpoint(y_absl::string_view peer_uri) {
    endpoint_.SetPeer(peer_uri);
  }

  void AddPropertyToAuthContext(const char* name, const char* value) {
    auth_context_.add_cstring_property(name, value);
  }

  EvaluateArgs MakeEvaluateArgs() {
    channel_args_ =
        new EvaluateArgs::PerChannelArgs(&auth_context_, &endpoint_);
    return EvaluateArgs(&metadata_, channel_args_);
  }

 private:
  ScopedArenaPtr arena_ = MakeScopedArena(1024);
  grpc_metadata_batch metadata_{arena_.get()};
  MockAuthorizationEndpoint endpoint_{/*local_uri=*/"", /*peer_uri=*/""};
  grpc_auth_context auth_context_{nullptr};
  EvaluateArgs::PerChannelArgs* channel_args_ = nullptr;
};

}  // namespace grpc_core

#endif  // GRPC_TEST_CORE_UTIL_EVALUATE_ARGS_TEST_UTIL_H
