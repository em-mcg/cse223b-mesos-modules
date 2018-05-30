#include "http_endpoint.hpp"
#include <process/http.hpp>
#include <process/process.hpp>
#include <mesos/mesos.hpp>

using namespace process;
using namespace process::http;

using mesos::MasterInfo;

namespace multimesos {

// endpoint that will return MasterInfo
static const std::string MASTER_INFO_ENDPOINT = "/detect_multimesos";

// libprocess registers endpoints as <protocol>://<ip>:<port>/<process_id>/<endpoint>
static const std::string CONTENDER_HTTP_PROCESS = "master-contender-http";

static const std::string PROTOBUF_CONTENT_TYPE = "application/octet-stream";

// new process with ID CONTENDER_HTTP_PROCESS
ContenderHttpProcess::ContenderHttpProcess(const mesos::MasterInfo& masterInfo) :
		ProcessBase(CONTENDER_HTTP_PROCESS), masterInfo(&masterInfo) {
}

void ContenderHttpProcess::initialize() {
	// register a new endpoint
	LOG(INFO) << "Installing http://" << self().address.ip  << ":" << self().address.port
			  << "/" + self().id << MASTER_INFO_ENDPOINT;

	// endpoint will return MasterInfo as a protobuf byte string
	route(MASTER_INFO_ENDPOINT,
			None(),
			[this](const process::http::Request& request) {
				// serialize the protobuf as a string
				std::string byteMessage = this->masterInfo->SerializeAsString();
				// return 200 with the string
				return OK(byteMessage, PROTOBUF_CONTENT_TYPE);
			});
}

ContenderHttp::ContenderHttp(const mesos::MasterInfo& masterInfo) :
		masterInfo(&masterInfo), process(new ContenderHttpProcess(masterInfo)) {
	spawn(process.get());
}

ContenderHttp::~ContenderHttp() {
	LOG(INFO) << "Terminating";
	terminate(process.get());
	wait(process.get());
}

std::string ContenderHttp::getMasterInfoPath() {
	return "/" + CONTENDER_HTTP_PROCESS + MASTER_INFO_ENDPOINT;
}

}
 // namespace multimesos {
