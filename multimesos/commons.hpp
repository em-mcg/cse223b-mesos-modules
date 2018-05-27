#ifndef __MULTI_MESOS_COMMONS_HPP__
#define __MULTI_MESOS_COMMONS_HPP__


#include <mesos/mesos.hpp>

#include <mesos/master/contender.hpp>

#include <process/future.hpp>

#include <stout/nothing.hpp>

using namespace mesos;
using process::UPID;

namespace multi_master_commons {

std::vector<MasterInfo::Capability> MASTER_CAPABILITIES();

MasterInfo createMasterInfo(const UPID& pid);

}

#endif // __MULTI_MESOS_COMMONS_HPP__
