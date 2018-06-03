cmake_minimum_required(VERSION 3.10)
project(modules)

set(CMAKE_CXX_STANDARD 11)

include_directories(authentication/cram_md5)
include_directories(isolator)
include_directories(multimesos)

add_executable(modules
        authentication/cram_md5/authenticatee.hpp
        authentication/cram_md5/authenticator.hpp
        authentication/cram_md5/auxprop.cpp
        authentication/cram_md5/auxprop.hpp
        authentication/test_authentication_modules.cpp
        hook/test_hook_module.cpp
        isolator/test_isolator_module.cpp
        isolator/test_isolator_module.hpp
        multimesos/commons.cpp
        multimesos/commons.hpp
        multimesos/http_endpoint.cpp
        multimesos/http_endpoint.hpp
        multimesos/list_map.hpp
        multimesos/multi_master_contender_module.cpp
        multimesos/multi_master_contender_module.hpp
        multimesos/multi_master_detector_module.cpp
        multimesos/multi_master_detector_module.hpp
        multimesos/multi_mesos_modules.cpp)
