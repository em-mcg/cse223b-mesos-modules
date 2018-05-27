// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __MULTI_MASTER_DETECTOR_MODULE_HPP__
#define __MULTI_MASTER_DETECTOR_MODULE_HPP__

#include <string>

#include <mesos/mesos.hpp>

#include <mesos/master/detector.hpp>

#include <process/future.hpp>

#include <stout/option.hpp>

using namespace mesos;
using namespace mesos::master::detector;

namespace multimesos {

// Forward declarations.
class MultiMasterDetectorProcess;

// A standalone implementation of the MasterDetector with no external
// discovery mechanism so the user has to manually appoint a leader
// to the detector for it to be detected.
class MultiMasterDetector : public MasterDetector
{
public:
  MultiMasterDetector();
  // Use this constructor if the leader is known beforehand so it is
  // unnecessary to call 'appoint()' separately.
  explicit MultiMasterDetector(const MasterInfo& leader);

  // Same as above but takes UPID as the parameter.
  explicit MultiMasterDetector(const process::UPID& leader);

  virtual ~MultiMasterDetector();

  // Appoint the leading master so it can be *detected*.
  void appoint(const Option<MasterInfo>& leader);

  // Same as above but takes 'UPID' as the parameter.
  void appoint(const process::UPID& leader);

  virtual process::Future<Option<MasterInfo>> detect(
      const Option<MasterInfo>& previous = None());

private:
  MultiMasterDetectorProcess* process;
};

} // namespace multimesos

#endif // __MULTI_MASTER_DETECTOR_MODULE_HPP__