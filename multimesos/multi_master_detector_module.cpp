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

#include "multi_master_detector_module.hpp"
#include "commons.hpp"

#include <set>

#include <mesos/master/detector.hpp>
#include <mesos/type_utils.hpp>

#include <process/defer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/id.hpp>
#include <process/process.hpp>

using namespace process;
using namespace mesos;
using namespace mesos::master::detector;

using std::set;
using process::defer;
using process::Future;

namespace multimesos {

class MultiMasterDetectorProcess
  : public Process<MultiMasterDetectorProcess>
{
public:
  MultiMasterDetectorProcess()
    : ProcessBase(ID::generate("multi-master-detector")) {}
  explicit MultiMasterDetectorProcess(const MasterInfo& _leader)
    : ProcessBase(ID::generate("multi-master-detector")),
      leader(_leader) {}

  ~MultiMasterDetectorProcess()
  {
    discardPromises(&promises);
  }

  void appoint(const Option<MasterInfo>& leader_)
  {
    leader = leader_;

    setPromises(&promises, leader);
  }

  Future<Option<MasterInfo>> detect(
      const Option<MasterInfo>& previous = None())
  {
    if (leader != previous) {
      return leader;
    }

    Promise<Option<MasterInfo>>* promise = new Promise<Option<MasterInfo>>();

    promise->future()
      .onDiscard(defer(self(), &Self::discard, promise->future()));

    promises.insert(promise);
    return promise->future();
  }

private:
  void discard(const Future<Option<MasterInfo>>& future)
  {
    // Discard the promise holding this future.
    discardPromises(&promises, future);
  }

  Option<MasterInfo> leader; // The appointed master.
  set<Promise<Option<MasterInfo>>*> promises;
};


MultiMasterDetector::MultiMasterDetector()
{
  process = new MultiMasterDetectorProcess();
  spawn(process);
}


MultiMasterDetector::MultiMasterDetector(const MasterInfo& leader)
{
  process = new MultiMasterDetectorProcess(leader);
  spawn(process);
}


MultiMasterDetector::MultiMasterDetector(const UPID& leader)
{
  process = new MultiMasterDetectorProcess(
      multi_master_commons::createMasterInfo(leader));

  spawn(process);
}


MultiMasterDetector::~MultiMasterDetector()
{
  terminate(process);
  process::wait(process);
  delete process;
}


void MultiMasterDetector::appoint(const Option<MasterInfo>& leader)
{
  dispatch(process, &MultiMasterDetectorProcess::appoint, leader);
}


void MultiMasterDetector::appoint(const UPID& leader)
{
  dispatch(process,
           &MultiMasterDetectorProcess::appoint,
		   multi_master_commons::createMasterInfo(leader));
}


Future<Option<MasterInfo>> MultiMasterDetector::detect(
    const Option<MasterInfo>& previous)
{
  return dispatch(process, &MultiMasterDetectorProcess::detect, previous);
}

} // namespace multimesos {
