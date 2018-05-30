
#include <stout/strings.hpp>

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/master/contender.hpp>
#include <mesos/module/contender.hpp>

#include <mesos/master/detector.hpp>
#include <mesos/module/detector.hpp>

#include "multi_master_contender_module.hpp"
#include "multi_master_detector_module.hpp"

using namespace multimesos;

http::URL* parseAddresses(std::map<std::string, std::string> pMap);
std::map<std::string, std::string> parseParameters(const Parameters& parameters);


mesos::modules::Module<MasterContender> org_apache_mesos_MultiMesosContender(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Erin McGinnis",
    "emmcginn@ucsd.edu",
    "MultiMaster Contender",
    nullptr,
    [](const Parameters& parameters) -> MasterContender* {
	  std::map<std::string, std::string> pMap = parseParameters(parameters);
	  http::URL* urls = parseAddresses(pMap);

      return new MultiMasterContender(urls);
    });


mesos::modules::Module<MasterDetector> org_apache_mesos_MultiMesosDetector(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Erin McGinnis",
    "emmcginn@ucsd.edu",
    "MultiMaster Detector",
    nullptr,
    [](const Parameters& parameters) -> MasterDetector* {
	  std::map<std::string, std::string> pMap = parseParameters(parameters);
	  http::URL *urls = parseAddresses(pMap);

      return new MultiMasterDetector(urls);
    });


std::map<std::string, std::string> parseParameters(const Parameters& parameters) {
	std::map<std::string, std::string> paramMap;

	foreach (const Parameter& parameter, parameters.parameter()) {
		paramMap[parameter.key()] = parameter.value();
	}

	return paramMap;
}

http::URL* parseAddresses(std::map<std::string, std::string> pMap) {
	std::string schemaDelim = "://";
	std::vector<std::string> leaders = strings::split(pMap["leaders"], ",");
	std::string schema = strings::split(leaders[0], schemaDelim)[0];
	leaders[0] = leaders[0].substr(schema.length() + schemaDelim.length());

	http::URL* urls = new http::URL[leaders.size()];

	for (int i = 0; i < int(leaders.size()); i++) {
		LOG(INFO) << schema + "://" + leaders[i];
		Try<http::URL> turl = http::URL::parse(schema + "://" + leaders[i]);

		if (turl.isError()) {
			LOG(FATAL) << "Module file contained bad leader address " << leaders[i]
					   << ". " << turl.error();
		}
		urls[i] = turl.get();
	}

	return urls;
}
