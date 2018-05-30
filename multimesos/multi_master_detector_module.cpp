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
#include "http_endpoint.hpp"
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
// using namespace process::http;

using namespace mesos;
using namespace mesos::master::detector;

using std::set;
using process::defer;
using process::Future;

namespace multimesos {

MultiMasterDetectorProcess::MultiMasterDetectorProcess() :
		ProcessBase(ID::generate("multi-master-detector")), leaderUrls(nullptr) {
}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(
		const MasterInfo& _leader) :
		ProcessBase(ID::generate("multi-master-detector")), leader(_leader),
		leaderUrls(nullptr) {
}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(http::URL* urls) :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(urls) {
}


MultiMasterDetectorProcess::~MultiMasterDetectorProcess() {
	discardPromises(&promises);
}

void MultiMasterDetectorProcess::appoint(const Option<MasterInfo>& leader_) {
	leader = leader_;

	setPromises(&promises, leader);
}

Future<Option<MasterInfo>> MultiMasterDetectorProcess::detect(
		const Option<MasterInfo>& previous) {
	Promise<Option<MasterInfo>>* promise = new Promise<Option<MasterInfo>>();

	// if there is a leader, continue talking to it
	// TODO: detect failure
	if (leader.isSome()) {
		// return a pending promise
		return promise->future();
	}

	// otherwise, find a leader
	getMasterInfo(leaderUrls[0]);

	promise->future().onDiscard(
			defer(self(), &Self::discard, promise->future()));

	promises.insert(promise);
	return promise->future();
}


http::URL chooseMaster(http::URL *urls, http::URL currentUrl) {
	return currentUrl;
}


void MultiMasterDetectorProcess::discard(
		const Future<Option<MasterInfo>>& future) {
	// Discard the promise holding this future.
	discardPromises(&promises, future);
}

void MultiMasterDetectorProcess::getMasterInfo(http::URL url) {
	// master should be an address of the form <protocol>://<ip|hostname>:<port>
	//Try<http::URL> url = http::URL::parse(master);

	url.path = ContenderHttp::getMasterInfoPath();

	// send message and wait for response
	Future<http::Response> future = http::get(url);

	// future.onAny(defer(self(), &MultiMasterDetectorProcess::parseMasterInfoResponse));

	future.onAny(defer(self(), [this](const Future<http::Response>& res) {
		MasterInfo* mInfo = new MasterInfo();
		std::string byteMessage = res.get().body;

		// parse bytes into MasterInfo protocol buffer
			mInfo->ParseFromString(byteMessage);

			// fulfill the promise
			appoint(Option<MasterInfo>::some(*mInfo));
		}));

	// future.onAny(&MultiMasterDetectorProcess::parseMasterInfo);
}


MultiMasterDetector::MultiMasterDetector() {
	process = new MultiMasterDetectorProcess();
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(http::URL* urls) {
	process = new MultiMasterDetectorProcess(urls);
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(const MasterInfo& masterInfo) {
	LOG(INFO)<< "Initializing MasterDetector at " << masterInfo.hostname() << ":" << masterInfo.port();
	process = new MultiMasterDetectorProcess(masterInfo);
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(const UPID& leader) {
	LOG(INFO)<< "Initializing MasterDetector at " << leader.address;
	process = new MultiMasterDetectorProcess(
			multi_master_commons::createMasterInfo(leader));

	spawn(process);
}

MultiMasterDetector::~MultiMasterDetector() {
	terminate(process);
	process::wait(process);
	delete process;
}

void MultiMasterDetector::appoint(const Option<MasterInfo>& leader) {
	dispatch(process, &MultiMasterDetectorProcess::appoint, leader);
}

void MultiMasterDetector::appoint(const UPID& leader) {
	dispatch(process, &MultiMasterDetectorProcess::appoint,
			multi_master_commons::createMasterInfo(leader));
}

Future<Option<MasterInfo>> MultiMasterDetector::detect(
		const Option<MasterInfo>& previous) {
	return dispatch(process, &MultiMasterDetectorProcess::detect, previous);
}

} // namespace multimesos {
