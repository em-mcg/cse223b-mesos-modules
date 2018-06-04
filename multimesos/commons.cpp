#include "commons.hpp"

#include <stout/uuid.hpp>
#include <process/process.hpp>

using namespace mesos;
using namespace mesos::master;

using process::UPID;
using std::string;

/**
 * Common utilities needed by other module classes
 */
namespace multimesos {

namespace commons {

// create a MasterInfo object from a pid
// stolen from a mesos file and put here because the
// original function isn't available through libmesos
MasterInfo createMasterInfo(const UPID& pid)
{
  MasterInfo info;
  info.set_id(stringify(pid) + "-" + id::UUID::random().toString());

  // NOTE: Currently, we store the ip in network order, which should
  // be fixed. See MESOS-1201 for more details.
  // TODO(marco): `ip` and `port` are deprecated in favor of `address`;
  //     remove them both after the deprecation cycle.
  info.set_ip(pid.address.ip.in()->s_addr);
  info.set_port(pid.address.port);

  info.mutable_address()->set_ip(stringify(pid.address.ip));
  info.mutable_address()->set_port(pid.address.port);

  info.set_pid(pid);

  Try<string> hostname = net::getHostname(pid.address.ip);
  if (hostname.isSome()) {
    // Hostname is deprecated; but we need to update it
    // to maintain backward compatibility.
    // TODO(marco): Remove once we deprecate it.
    info.set_hostname(hostname.get());
    info.mutable_address()->set_hostname(hostname.get());
  }

  foreach (const MasterInfo::Capability& capability,
           MASTER_CAPABILITIES()) {
    info.add_capabilities()->CopyFrom(capability);
  }

  return info;
}

// also stolen from a mesos file because it can't be accessed through libmesos
std::vector<MasterInfo::Capability> MASTER_CAPABILITIES()
{
  MasterInfo::Capability::Type types[] = {
    MasterInfo::Capability::AGENT_UPDATE,
  };

  std::vector<MasterInfo::Capability> result;
  foreach (MasterInfo::Capability::Type type, types) {
    MasterInfo::Capability capability;
    capability.set_type(type);
    result.push_back(capability);
  }

  return result;
}


// convert a URL to a string
std::string URLtoString(process::http::URL url) {
	std::stringstream buffer;

	buffer << url;
	std::string str = buffer.str();

	if (str.back() == '/') {
		str.pop_back();
	}

	return str;
}


// modulus that handles negative numbers
int modulus(int a, int b) {
	return (b + (a % b)) % b;
}

} // namespace commons {

} // namespace multimesos {

