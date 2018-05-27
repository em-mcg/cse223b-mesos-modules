
#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/master/contender.hpp>
#include <mesos/module/contender.hpp>

#include <mesos/master/detector.hpp>
#include <mesos/module/detector.hpp>

#include "multi_master_contender_module.hpp"
#include "multi_master_detector_module.hpp"

using namespace multimesos;

mesos::modules::Module<MasterContender> org_apache_mesos_MultiMesosContender(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Erin McGinnis",
    "emmcginn@ucsd.edu",
    "MultiMaster Contender",
    nullptr,
    [](const Parameters& parameters) -> MasterContender* {
      return new MultiMasterContender();
    });


mesos::modules::Module<MasterDetector> org_apache_mesos_MultiMesosDetector(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Erin McGinnis",
    "emmcginn@ucsd.edu",
    "MultiMaster Detector",
    nullptr,
    [](const Parameters& parameters) -> MasterDetector* {
      return new MultiMasterDetector();
    });
