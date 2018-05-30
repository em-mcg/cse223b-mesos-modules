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

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/master/contender.hpp>

#include <mesos/module/contender.hpp>

#include <stout/try.hpp>

// #include <mesos/master/contender/standalone.hpp>

#include "multi_master_contender_module.hpp"

#include "http_endpoint.hpp"

#include <mesos/master/contender.hpp>

#include <process/future.hpp>

using namespace process;
using namespace mesos;
using namespace mesos::master::contender;

namespace multimesos {


MultiMasterContender::MultiMasterContender()
: initialized(false),
  promise(nullptr),
  masterInfo(nullptr),
  infoEndpoint(nullptr),
  leaderUrls(nullptr)
  {}

MultiMasterContender::MultiMasterContender(http::URL* urls)
: initialized(false),
  promise(nullptr),
  masterInfo(nullptr),
  infoEndpoint(nullptr),
  leaderUrls(urls)
  {}

MultiMasterContender::~MultiMasterContender()
{
  if (promise != nullptr) {
    promise->set(Nothing()); // Leadership lost.
    delete promise;
  }

  if (infoEndpoint != nullptr) {
	delete infoEndpoint;
  }
}


void MultiMasterContender::initialize(const MasterInfo& masterInfo)
{
  // We don't really need to store the master in this basic
  // implementation so we just restore an 'initialized' flag to make
  // sure it is called.

  // set up master info endpoint
  this->infoEndpoint = new ContenderHttp(masterInfo);

  this->masterInfo = &masterInfo;
  initialized = true;

  LOG(INFO) << "Initialized MasterContender at " << this->contenderAddress();
}


Future<Future<Nothing>> MultiMasterContender::contend()
{
  LOG(INFO) << "Contending for leadership";
  if (!initialized) {
    return Failure("Initialize the contender first");
  }

  if (promise != nullptr) {
    LOG(INFO) << "Withdrawing the previous membership before recontending";
    promise->set(Nothing());
    delete promise;
  }

  // Directly return a future that is always pending because it
  // represents a membership/leadership that is not going to be lost
  // until we 'withdraw'.
  promise = new Promise<Nothing>();
  return promise->future();
}


std::string MultiMasterContender::contenderAddress() {
	return this->masterInfo->hostname() + ":" + std::to_string(this->masterInfo->port());
}

} // namespace multimesos {

// [] is the capture expression saying which variables
// from outside context are available in lambda expression
//
// () has parameters for lambda
// -> <type> { .. lambda expressions ... } is the return type and body
//
