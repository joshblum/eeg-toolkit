#ifndef VISGOTH_H
#define VISGOTH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "collectd.hpp"
#include "../json11/json11.hpp"

using namespace std;
using namespace json11;

class Visgoth
{
  private:
    Collectd collectd = Collectd();
    unordered_map<string, string> collectd_keys {
        {"memory", "memory-free"},
        {"cpu", "cpu-user"},
        {"network", "if_octets"},
        {"disk", "disk_octets"},
    };

  public:
    Visgoth();
    uint get_extent(Json profile_data);
    Json get_collectd_stats();
};

#endif // VISGOTH_H

