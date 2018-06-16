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

#ifndef __RANDOM_MASTER_MODULE_HPP__
#define __RANDOM_MASTER_MODULE_HPP__

#include <string>

#include <mesos/mesos.hpp>

#include <mesos/master/detector.hpp>

#include <process/future.hpp>

#include <process/process.hpp>

#include <stout/option.hpp>

#include "list_map.hpp"

#include "constants.hpp"

using namespace mesos;
using namespace process;
using namespace mesos::master::detector;

using std::set;

namespace multimesos {

// Forward declarations.
class RandomDetectorProcess;

// A standalone implementation of the MasterDetector with no external
// discovery mechanism so the user has to manually appoint a leader
// to the detector for it to be detected.
class RandomMasterDetector : public MasterDetector
{
public:
  // Use this constructor if the leader is known beforehand so it is
  // unnecessary to call 'appoint()' separately.
  explicit RandomMasterDetector(const MasterInfo& leader);

  // Give detector a list of master URLs
  explicit RandomMasterDetector(UrlListMap* urls);

  // Same as above but takes UPID as the parameter.
  explicit RandomMasterDetector(const process::UPID& leader);

  // Destructor
  virtual ~RandomMasterDetector();

  // Appoint the leading master so it can be *detected*.
  void appoint(const Option<MasterInfo>& leader);

  // Same as above but takes 'UPID' as the parameter.
  void appoint(const process::UPID& leader);

  virtual process::Future<Option<MasterInfo>> detect(
      const Option<MasterInfo>& previous = None());

private:
  RandomDetectorProcess* process;
};


class RandomDetectorProcess
  : public Process<RandomDetectorProcess>
{
public:
  RandomDetectorProcess();

  RandomDetectorProcess(UrlListMap* urls);

  explicit RandomDetectorProcess(const MasterInfo& _leader);

  ~RandomDetectorProcess();

  void appoint(const Option<MasterInfo>& leader_);

  void appoint(const Option<MasterInfo>& leader_, int leaderIndex);

  void initialize();

  Future<Option<MasterInfo>> detect(
      const Option<MasterInfo>& previous = None());

  //void getMasterInfo(std::string master);
  void getMasterInfo(int leaderIndex);

  void setAddress();

  int chooseHash(UrlListMap* urls, http::URL currentURL);

  int chooseMaster(UrlListMap* urls, http::URL currentUrl);

private:
  void discard(const Future<Option<MasterInfo>>& future);

  void sendHeartBeats();

  void sendHeartBeat(int index);

  void receiveHeartBeat();

  void heartBeatFailure(int index, Duration maxBackoff);

  void maxHeartbeatFailure(int index);

  // the appointed master
  Option<MasterInfo> leader;

  // a set of promises returned by the detector
  set<Promise<Option<MasterInfo>>*> promises;

  // list of all known master URLs
  UrlListMap* leaderUrls;

  // the address of `this` process
  http::URL address;

  // set when the process destructor is called
  bool shuttingDown = false;

  int* failedPings = nullptr;

  process::Timer* masterHeartbeatTimers = nullptr;

  process::Timer heartBeatTimer;

  // set when the process has been initialized
  bool initialized = false;

  std::atomic<bool> detecting;

  int count = 0;

  // current master index
  int mIndex = 0;
};

} // namespace multimesos

#endif // __RANDOM_MASTER_MODULE_HPP__
