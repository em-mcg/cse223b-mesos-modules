
#ifndef __HTTP_ENDPOINT_HPP__
#define __HTTP_ENDPOINT_HPP__

#include <process/http.hpp>
#include <process/process.hpp>
#include <process/protobuf.hpp>
#include <mesos/mesos.hpp>
#include <mesos/type_utils.hpp>

using namespace process;
using namespace process::http;

using mesos::MasterInfo;

namespace multimesos {

class ContenderHttpProcess : public Process<ContenderHttpProcess>
{

public:
  ContenderHttpProcess(const mesos::MasterInfo& masterInfo);

protected:
  virtual void initialize();

private:
  const mesos::MasterInfo* masterInfo;
};


class ContenderHttp
{
public:
  ContenderHttp(const mesos::MasterInfo& masterInfo);
  virtual ~ContenderHttp();

  static std::string getMasterInfoPath();

private:
  const mesos::MasterInfo* masterInfo;
  Owned<ContenderHttpProcess> process;
};

} // namespace multimesos {


#endif // __HTTP_ENDPOINT_HPP__
