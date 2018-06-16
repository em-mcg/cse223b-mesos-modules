# Contender Module

 - multi_master_contender_module.{h,c}pp contain mesos contender code.
 
 The MultiMasterContender is nearly identical to Mesos's original standalone contender,
 except that it starts an additional HTTP detector endpoint through libprocess.
 
 The contender will always tell the master process running it that it is the leading master.
 
# Detector Module
 
 The following files contain detector code:
 
 - random_detector_module.{h,c}pp will try to connect to random masters.
 - sequential_detector_module.{h,c}pp will iterate through a list of masters, attempting
  to connect to each one.
  
# Module Declaration

 - multi_mesos_modules.cpp contains the module code that Mesos will load. These classes
  should be specified in a `modules.json` file.
  
