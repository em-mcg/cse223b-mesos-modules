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

#ifndef __MULTI_MASTER_CONTENDER_HPP__
#define __MULTI_MASTER_CONTENDER_HPP__

#include <mesos/mesos.hpp>

#include <mesos/master/contender.hpp>

#include <process/future.hpp>

#include <stout/nothing.hpp>

#include "http_endpoint.hpp"

using namespace mesos;
using namespace mesos::master::contender;

namespace multimesos {

// Multiple masters will be successful contenders
class MultiMasterContender : public MasterContender
{
public:
  MultiMasterContender();

  MultiMasterContender(http::URL* urls);

  ~MultiMasterContender();

  // MasterContender implementation.
  void initialize(const MasterInfo& masterInfo);

  // In this basic implementation the outer Future directly returns
  // and inner Future stays pending because there is only one
  // contender in the contest.
  process::Future<process::Future<Nothing>> contend();

  std::string contenderAddress();

private:
  bool initialized;
  process::Promise<Nothing>* promise;
  const MasterInfo* masterInfo;
  ContenderHttp* infoEndpoint;
  http::URL *leaderUrls;
};

} // namespace multimesos {


#endif // __MULTI_MASTER_CONTENDER_HPP__
