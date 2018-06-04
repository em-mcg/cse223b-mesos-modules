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

// constructors - TODO: can probably get rid of some of these constructors unless we find a use case
MultiMasterDetectorProcess::MultiMasterDetectorProcess() :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(nullptr),
		shuttingDown(false),
		initialized(false)
{}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(
		const MasterInfo& _leader) :
		ProcessBase(ID::generate("multi-master-detector")), leader(_leader),
		leaderUrls(nullptr),
		shuttingDown(false),
		initialized(false)
{}

MultiMasterDetectorProcess::MultiMasterDetectorProcess(UrlListMap* urls) :
		ProcessBase(ID::generate("multi-master-detector")),
		leaderUrls(urls),
		shuttingDown(false),
		initialized(false) {}


MultiMasterDetectorProcess::~MultiMasterDetectorProcess()
{
	// kill the detector promise
	shuttingDown = true;
	discardPromises(&promises);
}


void MultiMasterDetectorProcess::initialize() {
	// initialize the detector process
	LOG(INFO) << "Initializing master detector process";
	setAddress();
	initialized = true;
}


void MultiMasterDetectorProcess::appoint(const Option<MasterInfo>& leader_) {
	// save the leader we elected and resolve all pending promises with the leader
	leader = leader_;
	setPromises(&promises, leader);
}

// detect a Mesos master
Future<Option<MasterInfo>> MultiMasterDetectorProcess::detect(
		const Option<MasterInfo>& previous) {
	// if not initialized, the url lists won't be set up and it'd crash somewhere
	if (!initialized) {
		return Failure("Initialize the detector first");
	}

	// promise master info; will be returned to `master`, `slave`, or `framework`
	Promise<Option<MasterInfo>>* promise = new Promise<Option<MasterInfo>>();

	// set up a discard handler; if anyone calls `discard` on `promise`, this handler
	// will be called
	promise->future().onDiscard(
			defer(self(), &Self::discard, promise->future()));
	// add to promise set
	promises.insert(promise);

	// if there is a leader, continue talking to it
	if (leader.isSome()) {
		// return a pending promise so detect isn't called continuously
		return promise->future();
	}

	// otherwise, find a leader
	http::URL newLeader = chooseMaster(this->leaderUrls, this->address);

	// get the info for that leader; this method will also appoint the master
	getMasterInfo(newLeader);

	// return what should be a `set` promise
	return promise->future();
}

void MultiMasterDetectorProcess::setAddress() {
	// once the `DetectorProcess` has been initialized, store full process
	// address for convenience
	std::stringstream buffer;

	buffer << "http://" << this->self().address.ip << ":" <<
			this->self().address.port;

	LOG(INFO) << "Detector process address is " << buffer.str();
	address = http::URL::parse(buffer.str()).get();
}


http::URL MultiMasterDetectorProcess::chooseMaster(UrlListMap* urls, http::URL currentURL) {
	// if this node is a `master` (i.e. its URL is in the url list), choose self
	if (urls->contains(currentURL)) {
    	return currentURL;
    }

	// otherwise, choose a master from the url list
    return chooseHash(urls, currentURL);
}


http::URL MultiMasterDetectorProcess::chooseHash(UrlListMap* urls, http::URL currentURL) {
	// hash a URL and choose a new master
	// URL may be `this` node's URL or that of the previous master
    std::string url = commons::URLtoString(currentURL);
    std::hash<std::string> hasher;
    int hashed = std::abs((int)hasher(url));

    // some checks just in case
    CHECK((hashed % urls->length()) < urls->length());
    CHECK((hashed % urls->length()) >= 0);

    // return the URL; TODO: could do this differently
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
	// use a contender method to get the master endpoint and set it as the URL path
	url.path = ContenderHttp::getMasterInfoPath();

	// send message and wait for response
	LOG(INFO) << "Sent GET request to detect endpoint " << url;

	// ask the endpoint for master info
	Future<http::Response> future = http::get(url);

	future.onReady(defer(self(), [this](const Future<http::Response>& res) {
			// set up a new MI object and populate it with protocol buffer bytes
			MasterInfo* mInfo = new MasterInfo();
			std::string byteMessage = res.get().body;

		    // parse bytes into MasterInfo protocol buffer
			mInfo->ParseFromString(byteMessage);

			// fulfill the promise
			appoint(Option<MasterInfo>::some(*mInfo));
		})).onFailed(lambda::bind([this, url](const std::string& message, const std::string& failure) {
			// GET request failed
			// show error and choose a new master to contact
			LOG(WARNING) << message << " " << failure;
			http::URL newURL = MultiMasterDetectorProcess::chooseHash(this->leaderUrls, url);
			LOG(INFO) << "Attempting to connect to leader " << newURL;
			getMasterInfo(newURL);
	}, "Failed to contact master", lambda::_1));

	// TODO: figure out benefits of onAny(defer([callback])) vs onAny([callback])
	// future.onAny(&MultiMasterDetectorProcess::parseMasterInfo);
}


MultiMasterDetector::MultiMasterDetector() {
	// spawn the detector process
	process = new MultiMasterDetectorProcess();
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(UrlListMap* urls) {
	// spawn the detector process with a list of master URLs
	process = new MultiMasterDetectorProcess(urls);
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(const MasterInfo& masterInfo) {
	// spawn detector using a master we're already aware of
	// basically gives standalone master semantics to this class
	LOG(INFO)<< "Initializing MasterDetector at " << masterInfo.hostname() << ":" << masterInfo.port();
	process = new MultiMasterDetectorProcess(masterInfo);
	spawn(process);
}

MultiMasterDetector::MultiMasterDetector(const UPID& leader) {
	// spawn detector using a master we're already aware of
	// basically gives standalone master semantics to this class
	LOG(INFO)<< "Initializing MasterDetector at " << leader.address;
	process = new MultiMasterDetectorProcess(
			commons::createMasterInfo(leader));
	spawn(process);
}

MultiMasterDetector::~MultiMasterDetector() {
	// cleanup
	// TODO: there's definitely more cleanup to do
	terminate(process);
	process::wait(process);
	delete process;
}

void MultiMasterDetector::appoint(const Option<MasterInfo>& leader) {
	// we could directory appoint a master from the detector class
	// normally the detector process will handle appointment
	dispatch(process, &MultiMasterDetectorProcess::appoint, leader);
}

void MultiMasterDetector::appoint(const UPID& leader) {
	// we could directory appoint a master from the detector class
	// normally the detector process will handle appointment
	dispatch(process, &MultiMasterDetectorProcess::appoint,
			commons::createMasterInfo(leader));
}

Future<Option<MasterInfo>> MultiMasterDetector::detect(
		const Option<MasterInfo>& previous) {
	// detect a new master given the previous master (which may be None())
	return dispatch(process, &MultiMasterDetectorProcess::detect, previous);
}

} // namespace multimesos {
