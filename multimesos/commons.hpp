#ifndef __MULTI_MESOS_COMMONS_HPP__
#define __MULTI_MESOS_COMMONS_HPP__


#include <mesos/mesos.hpp>

#include <mesos/master/contender.hpp>

#include <process/future.hpp>

#include <process/process.hpp>

#include <stout/nothing.hpp>

using namespace mesos;
using process::UPID;

namespace multimesos {

namespace commons {

std::vector<MasterInfo::Capability> MASTER_CAPABILITIES();

MasterInfo createMasterInfo(const UPID& pid);

std::string URLtoString(process::http::URL url);

void gen_random(char *s, const int len);

} // namespace commons {

} // namespace multimesos {

#endif // __MULTI_MESOS_COMMONS_HPP__
