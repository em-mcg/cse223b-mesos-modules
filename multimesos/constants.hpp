#ifndef __MULTI_MESOS_CONSTANTS_HPP__
#define __MULTI_MESOS_CONSTANTS_HPP__

constexpr int DETECTOR_MAX_PING_TIMEOUT = 5;

constexpr Duration DEFAULT_PING_BACKOFF_FACTOR = Milliseconds(200);

const std::string MASTER_HEALTH_ENDPOINT = "/master/health";


#endif // __MULTI_MESOS_CONSTANTS_HPP__