#ifndef __LIST_MAP_HPP__
#define __LIST_MAP_HPP__

#include <unordered_set>
#include <vector>
#include <process/process.hpp>
#include <stout/net.hpp>

#include "commons.hpp"

using std::vector;
using std::unordered_set;

/**
 * Class with python-list like semantics
 */
namespace multimesos {

template <typename T, typename hash, typename equal>
class ListMap
{
public:

  // initialize class
  ListMap() {}

  // clone a listmap
  ListMap(const multimesos::ListMap<T, hash, equal>& lm) {
	  tset = lm.tset;
	  tvec = lm.tvec;
  }

  // initialize the list map
  ListMap(T* items, int numItems)
  {
	  // push all items to the map and vector
	  for (int i = 0; i < numItems; i++)
	  {
		  tvec.push_back(items[i]);
		  tset.insert(items[i]);
	  }
  }
  
  // check if an item is in the list
  bool contains(T item)
  {
	  return tset.find(item) != tset.end();
  }
  
  // return the index of an item in the list
  // or -1 if not found
  int index(T item)
  {
    for (int i = 0; i < tvec.size(); i++)
    {
      if (tvec[i] == item) {
        return i;
      }
    }

    return -1;
  }

  // get the number of items
  int length()
  {
	  return (int)tvec.size();
  }
  
  // access [] operator
  T operator [](int i) const
  {
	  return tvec[i];
  }

  // `get` because it's nicer for pointers to listmaps
  T get(int i) const
  {
    return tvec[i];
  }


private:
  // set to check if vector contains an item
  unordered_set<T, hash, equal> tset;
  // vector for indexed access to items
  vector<T> tvec;
};


template <typename T, typename hash, typename equal>
int len(ListMap<T, hash, equal> lm)
{
	// more python semantics
	return lm.length();
}

// need to tell the set how to hash certain objects
// other custom hash definitions should be added to this struct
struct ListMapHash {
   size_t operator() (const process::http::URL& url) const {
     std::string str = commons::URLtoString(url);
     std::hash<std::string> hasher;
     auto hashed = hasher(str);
     return hashed;
   }
};


inline bool operator== (process::http::URL const& lhs, process::http::URL const& rhs)
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

  // if domain names aren't equal, these URLs aren't equal
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
}

// need to tell the set how to compare two objects
// only need the equals operator since listmap uses an unordered_set
struct ListMapEqual {
  inline bool operator() (process::http::URL const& lhs, process::http::URL const& rhs) const
  {
	  return lhs == rhs;
  }
};

// long type needed to store URLs in this class
typedef ListMap<process::http::URL, ListMapHash, ListMapEqual> UrlListMap;

} // namespace multimesos {


#endif // __LIST_MAP_HPP__
