#ifndef __LIST_MAP_HPP__
#define __LIST_MAP_HPP__

#include <unordered_set>
#include <vector>
#include <process/process.hpp>
#include <stout/net.hpp>

#include "commons.hpp"

using std::vector;
using std::unordered_set;

namespace multimesos {

template <typename T, typename hash, typename equal>
class ListMap
{
public:

  ListMap() {

  }

  ListMap(const multimesos::ListMap<T, hash, equal>& lm) {
	  tset = lm.tset;
	  tvec = lm.tvec;
  }

  ListMap(T* items, int numItems)
  {
	  for (int i = 0; i < numItems; i++)
	  {
		  tvec.push_back(items[i]);
		  tset.insert(items[i]);
	  }
  }
  
  bool contains(T item)
  {
	  return tset.find(item) != tset.end();
  }
  
  int length()
  {
	  return (int)tvec.size();
  }
  
  T operator [](int i) const
  {
	  return tvec[i];
  }

  T get(int i) const
  {
    return tvec[i];
  }


private:
  unordered_set<T, hash, equal> tset;
  vector<T> tvec;
};


template <typename T, typename hash, typename equal>
int len(ListMap<T, hash, equal> lm)
{
	return lm.length();
}

struct ListMapHash {
   size_t operator() (const process::http::URL& url) const {
     std::string str = commons::URLtoString(url);
     std::hash<std::string> hasher;
     auto hashed = hasher(str);
     return hashed;
   }
};

struct ListMapEqual {
  inline bool operator() (process::http::URL const& lhs, process::http::URL const& rhs) const
  {
	  std::string rhsHost;
	  std::string lhsHost;

	  // try to resolve domain names
	  if (rhs.domain.isSome()) {
		  rhsHost = rhs.domain.get();
	  } else {
		  rhsHost = net::getHostname(rhs.ip.get()).get();
	  }

	  if (lhs.domain.isSome()) {
		  lhsHost = lhs.domain.get();
	  } else {
		  lhsHost = net::getHostname(lhs.ip.get()).get();
	  }

	  if (lhsHost != rhsHost) {
		  return false;
	  }

	  // check if ports are equal
	  if (lhs.port.isSome() && rhs.port.isSome()) {
		  if (lhs.port.get() == rhs.port.get()) {
			  return true;
		  }
	  }

	  // don't check protocol or path here
      return false;
  };

  //inline bool operator() (const process::http::URL& lhs, process::http::URL& rhs) {
  //  return (lhs.ip == rhs.ip && lhs.port == rhs.port);
  //};
};

typedef ListMap<process::http::URL, ListMapHash, ListMapEqual> UrlListMap;


} // namespace multimesos {


#endif // __LIST_MAP_HPP__
