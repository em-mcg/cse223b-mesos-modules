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

#include "sequential_detector_module.hpp"
#include "http_endpoint.hpp"
#include "commons.hpp"
#include "constants.hpp"

#include <set>

#include <mesos/master/detector.hpp>
#include <mesos/type_utils.hpp>
#include <mesos/mesos.hpp>

#include <process/defer.hpp>
#include <process/delay.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/id.hpp>
#include <process/process.hpp>

#include "list_map.hpp"

using namespace process;

using namespace mesos;
using namespace mesos::master::detector;

using std::set;
using process::defer;
using process::Future;

namespace multimesos {

// constructors - TODO: can probably get rid of some of these constructors unless we find a use case
SequentialDetectorProcess::SequentialDetectorProcess() :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(nullptr)
{}

SequentialDetectorProcess::SequentialDetectorProcess(
		const MasterInfo& _leader) :
		ProcessBase(ID::generate("multi-master-detector")),
		leader(_leader),
		leaderUrls(nullptr)
{}

SequentialDetectorProcess::SequentialDetectorProcess(
  UrlListMap* urls) :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(urls)
{}


SequentialDetectorProcess::~SequentialDetectorProcess()
{
	// kill the detector promise
	shuttingDown = true;
	discardPromises(&promises);
}


void SequentialDetectorProcess::initialize() {
	// initialize the detector process
	LOG(INFO) << "Initializing master detector process";
	setAddress();

	int numLeaders = leaderUrls->length();
	failedPings = new int[numLeaders];
	masterHeartbeatTimers = new process::Timer[numLeaders];
	//leaderPromises;

	for (int i = 0; i < numLeaders; i++) {
	  // initialize all ping counters to 0
	  failedPings[i] = 0;
	  leaderPromises.push_back(new Promise<Option<MasterInfo>>());
	}

	// don't skip first master
  mIndex = -1;

	// initialize
	initialized = true;

	detecting.store(false);

	// start heartbeat
	sendHeartBeats();
}


void SequentialDetectorProcess::sendHeartBeats() {
  // pause heartbeat while detecting
  if (!detecting.load()) {
    if (mIndex == leaderUrls->length() - 1) {
      // new detection cycle
      // detect all will try each master again
      // send a heartbeat to each leader
      for (int i = 0; i < leaderUrls->length(); i++) {
        sendHeartBeat(i);
      }
    }
  }

  // cancel timer just in case
  Clock::cancel(heartBeatTimer);

  // send another heartbeat after
  heartBeatTimer = process::delay(
        DEFAULT_PING_BACKOFF_FACTOR,
        self(),
        &SequentialDetectorProcess::sendHeartBeats);
}


void SequentialDetectorProcess::sendHeartBeat(int index) {
  http::URL url = leaderUrls->get(index);
  url.path = MASTER_HEALTH_ENDPOINT;

  Future<http::Response> future = get(url);
  future.onReady(defer(self(), [this, index](const Future<http::Response>& res) {
      // cancel clock and reset failedPing
      Clock::cancel(masterHeartbeatTimers[index]);
      failedPings[index] = 0;

    })).onFailed(lambda::bind([this, index, url](const std::string& message, const std::string& failure) {
      // go to failure function
      // LOG(INFO) << "Failed to contact master " << url << " with error " << failure << " " << message;
      heartBeatFailure(index, DEFAULT_PING_BACKOFF_FACTOR);
  }, "Failed to contact master", lambda::_1));

  // If this timer goes off, we didn't receive a response
  // within a reasonable amount of time
  masterHeartbeatTimers[index] = process::delay(
      DEFAULT_PING_BACKOFF_FACTOR,
      self(),
      &SequentialDetectorProcess::heartBeatFailure,
      index,
      DEFAULT_PING_BACKOFF_FACTOR/2);
}


void SequentialDetectorProcess::heartBeatFailure(int index, Duration maxBackoff) {
  // okay to cancel an already canceled/ended timer
  Clock::cancel(masterHeartbeatTimers[index]);

  // increment the number of failures
  failedPings[index]++;

  // if we've failed too many times, take some action
  if (failedPings[index] >= DETECTOR_MAX_PING_TIMEOUT) {
    maxHeartbeatFailure(index);
  }

  // send another heartbeat
  defer(self(), &Self::sendHeartBeat, index);
}


void SequentialDetectorProcess::maxHeartbeatFailure(int index) {
  mIndex = -1;
  detect(leader);
}


void SequentialDetectorProcess::appoint(const Option<MasterInfo>& leader_) {
  // save the leader we elected and resolve all pending promises with the leader
  setPromises(&promises, leader_);
}


void SequentialDetectorProcess::appoint(const Option<MasterInfo>& leader_, int leaderIndex) {
	// save the leader we elected and resolve all pending promises with the leader
  LOG(INFO) << "Appointing leader " << leader_.get().pid();
	leader = leader_;

	setPromises(&promises, leader_);

	leaderPromises[leaderIndex] = *(promises.begin());
}

// detect a Mesos master
Future<Option<MasterInfo>> SequentialDetectorProcess::detect(
		const Option<MasterInfo>& previous) {
	// if not initialized, the url lists won't be set up and it'd crash somewhere
	if (!initialized) {
		return Failure("Initialize the detector first");
	}

	detecting.store(true);

	// promise master info; will be returned to `master`, `slave`, or `framework`
	Promise<Option<MasterInfo>>* promise = new Promise<Option<MasterInfo>>();

	// set up a discard handler; if anyone calls `discard` on `promise`, this handler
	// will be called
	promise->future().onDiscard(
			defer(self(), &Self::discard, promise->future()));
	// add to promise set
	promises.insert(promise);

	// if we're at the end of the leader list, stop detecting
	if (mIndex == leaderUrls->length() - 1) {
		// return a pending promise so detect isn't called continuously
    detecting.store(false);
    return promise->future();
	}

  // try next leader
	int newLeaderIndex = chooseMaster(leaderUrls, address);
	getMasterInfo(newLeaderIndex);

	// return what should be a `set` promise
	detecting.store(false);
	return promise->future();
}

void SequentialDetectorProcess::setAddress() {
  // once the `DetectorProcess` has been initialized, store full process
  // address for convenience
  std::stringstream buffer;

  buffer << "http://" << this->self().address.ip << ":" <<
      this->self().address.port;

  LOG(INFO) << "Detector process address is " << buffer.str();
	address = http::URL::parse(buffer.str()).get();
}


int SequentialDetectorProcess::chooseMaster(UrlListMap* urls, http::URL currentURL) {
  // if this node is a `master` (i.e. its URL is in the url list), choose self
  if (urls->contains(currentURL)) {
      mIndex = urls->index(currentURL);
      return urls->index(currentURL);
  }

  mIndex = (mIndex + 1) % urls->length();
  return mIndex;
}


void SequentialDetectorProcess::discard(
		const Future<Option<MasterInfo>>& future) {
	LOG(INFO) << "Discarding promises";
	// Discard the promise holding this future.
	discardPromises(&promises, future);

	// promise was discarded but detector isn't shutting down
	if (!shuttingDown) {
		// otherwise, find a leader
	  // int newLeaderIndex = chooseRandomMaster(this->leaderUrls, this->address);
		getMasterInfo(mIndex);
	}
}

void SequentialDetectorProcess::getMasterInfo(int leaderIndex) {
	// master should be an address of the form <protocol>://<ip|hostname>:<port>
	// use a contender method to get the master endpoint and set it as the URL path
  http::URL url = leaderUrls->get(leaderIndex);

	url.path = ContenderHttp::getMasterInfoPath();

	// send message and wait for response
	LOG(INFO) << "Sent GET request to detect endpoint " << url;

	// ask the endpoint for master info
	Future<http::Response> future = http::get(url);

	future.onReady(defer(self(), [this, leaderIndex](const Future<http::Response>& res) {
			// set up a new MI object and populate it with protocol buffer bytes
			MasterInfo* mInfo = new MasterInfo();
			std::string byteMessage = res.get().body;

		    // parse bytes into MasterInfo protocol buffer
			mInfo->ParseFromString(byteMessage);

			// fulfill the promise
			appoint(Option<MasterInfo>::some(*mInfo), leaderIndex);
		})).onFailed(lambda::bind([this, url](const std::string& message, const std::string& failure) {
			// GET request failed
			// show error and choose a new master to contact
			LOG(WARNING) << message << " " << failure;

			int newLeaderIndex = chooseMaster(this->leaderUrls, this->address);

			LOG(INFO) << "Attempting to connect to leader " << newLeaderIndex;
			getMasterInfo(newLeaderIndex);
	}, "Failed to contact master", lambda::_1));

	// TODO: figure out benefits of onAny(defer([callback])) vs onAny([callback])
	// future.onAny(&SequentialDetectorProcess::parseMasterInfo);
}


SequentialMasterDetector::SequentialMasterDetector(UrlListMap* urls) {
	// spawn the detector process with a list of master URLs
	process = new SequentialDetectorProcess(urls);
	spawn(process);
}

SequentialMasterDetector::SequentialMasterDetector(const MasterInfo& masterInfo) {
	// spawn detector using a master we're already aware of
	// basically gives standalone master semantics to this class
	LOG(INFO)<< "Initializing MasterDetector at " << masterInfo.hostname() << ":" << masterInfo.port();
	process = new SequentialDetectorProcess(masterInfo);
	spawn(process);
}

SequentialMasterDetector::SequentialMasterDetector(const UPID& leader) {
	// spawn detector using a master we're already aware of
	// basically gives standalone master semantics to this class
	LOG(INFO)<< "Initializing MasterDetector at " << leader.address;
	process = new SequentialDetectorProcess(
			commons::createMasterInfo(leader));
	spawn(process);
}

SequentialMasterDetector::~SequentialMasterDetector() {
	// cleanup
	// TODO: there's definitely more cleanup to do
	terminate(process);
	process::wait(process);
	delete process;
}

void SequentialMasterDetector::appoint(const Option<MasterInfo>& leader) {
	// we could directory appoint a master from the detector class
	// normally the detector process will handle appointment

	dispatch(process, &SequentialDetectorProcess::appoint, leader);
}

void SequentialMasterDetector::appoint(const UPID& leader) {
	// we could directory appoint a master from the detector class
	// normally the detector process will handle appointment
	dispatch(process, &SequentialDetectorProcess::appoint,
			commons::createMasterInfo(leader));
}

Future<Option<MasterInfo>> SequentialMasterDetector::detect(
		const Option<MasterInfo>& previous) {
	// detect a new master given the previous master (which may be None())
	return dispatch(process, &SequentialDetectorProcess::detect, previous);
}

} // namespace multimesos {
