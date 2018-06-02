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

#include "list_map.hpp"

using namespace process;
// using namespace process::http;

using namespace mesos;
using namespace mesos::master::detector;

using std::set;
using process::defer;
using process::Future;

namespace multimesos {

MultiMasterDetectorProcess::MultiMasterDetectorProcess() :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(nullptr),
		shuttingDown(false)
{}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(
		const MasterInfo& _leader) :
		ProcessBase(ID::generate("multi-master-detector")), leader(_leader),
		leaderUrls(nullptr),
		shuttingDown(false)
{}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(UrlListMap* urls) :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(urls),
		shuttingDown(false) {}


MultiMasterDetectorProcess::~MultiMasterDetectorProcess()
{
	shuttingDown = true;
	discardPromises(&promises);
}


void MultiMasterDetectorProcess::initialize() {
	LOG(INFO) << "Initializing master detector process";
	setAddress();
}


void MultiMasterDetectorProcess::appoint(const Option<MasterInfo>& leader_) {
	leader = leader_;

	setPromises(&promises, leader);
}

Future<Option<MasterInfo>> MultiMasterDetectorProcess::detect(
		const Option<MasterInfo>& previous) {
	Promise<Option<MasterInfo>>* promise = new Promise<Option<MasterInfo>>();

	promise->future().onDiscard(
			defer(self(), &Self::discard, promise->future()));
	promises.insert(promise);

	// if there is a leader, continue talking to it
	// TODO: detect failure
	if (leader.isSome()) {
		// return a pending promise
		return promise->future();
	}

	// otherwise, find a leader
	http::URL newLeader = chooseMaster(this->leaderUrls, this->address);
	getMasterInfo(newLeader);

	return promise->future();
}

void MultiMasterDetectorProcess::setAddress() {
	std::stringstream buffer;

	buffer << "http://" << this->self().address.ip << ":" <<
			this->self().address.port;

	address = http::URL::parse(buffer.str()).get();
}


http::URL MultiMasterDetectorProcess::chooseMaster(UrlListMap* urls, http::URL currentURL) {
	if (urls->contains(currentURL)) {
    	return currentURL;
    }

    return chooseHash(urls, currentURL);
}


http::URL MultiMasterDetectorProcess::chooseHash(UrlListMap* urls, http::URL currentURL) {
    std::string url = commons::URLtoString(currentURL);
    std::hash<std::string> hasher;
    int hashed = (int)hasher(url);

    return urls->get(hashed % urls->length());
}


void MultiMasterDetectorProcess::discard(
		const Future<Option<MasterInfo>>& future) {
	LOG(INFO) << "Discarding promises";
	// Discard the promise holding this future.
	discardPromises(&promises, future);

	// promise was discarded but detector isn't shutting down
	if (!shuttingDown) {
		// otherwise, find a leader
		http::URL newLeader = chooseMaster(this->leaderUrls, this->address);
		getMasterInfo(newLeader);
	}
}

void MultiMasterDetectorProcess::getMasterInfo(http::URL url) {
	// master should be an address of the form <protocol>://<ip|hostname>:<port>
	//Try<http::URL> url = http::URL::parse(master);

	url.path = ContenderHttp::getMasterInfoPath();

	// send message and wait for response
	Future<http::Response> future = http::get(url);

	future.onReady(defer(self(), [this](const Future<http::Response>& res) {
			MasterInfo* mInfo = new MasterInfo();
			std::string byteMessage = res.get().body;

		    // parse bytes into MasterInfo protocol buffer
			mInfo->ParseFromString(byteMessage);

			// fulfill the promise
			appoint(Option<MasterInfo>::some(*mInfo));
		})).onFailed(lambda::bind([this, url](const std::string& message, const std::string& failure) {
					LOG(WARNING) << message << " " << failure;
					http::URL newURL = MultiMasterDetectorProcess::chooseHash(this->leaderUrls, url);
					LOG(INFO) << "Attempting to connect to leader " << newURL;
					getMasterInfo(newURL);
	}, "Failed to contact master", lambda::_1));
	// future.

	// else http::URL newLeader = chooseMaster(this->leaderUrls, this->address);

	// future.onAny(&MultiMasterDetectorProcess::parseMasterInfo);
}


MultiMasterDetector::MultiMasterDetector() {
	process = new MultiMasterDetectorProcess();
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(UrlListMap* urls) {
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
			commons::createMasterInfo(leader));

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
			commons::createMasterInfo(leader));
}

Future<Option<MasterInfo>> MultiMasterDetector::detect(
		const Option<MasterInfo>& previous) {
	return dispatch(process, &MultiMasterDetectorProcess::detect, previous);
}

} // namespace multimesos {
