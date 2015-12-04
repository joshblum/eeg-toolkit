#include "visgoth.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "sys/socket.h"
#include "collectd.hpp"

#include "../json11/json11.hpp"
#include "../helpers.hpp"

using namespace std;
using namespace json11;


Visgoth::Visgoth() {}

Json Visgoth::get_collectd_stats()
{
  unordered_map<string, float> float_values;
  vector<string> lines = collectd.list();
  for (auto l_it = lines.begin(); l_it != lines.end(); l_it++)
  {
    string list_key = split(*l_it, ' ')[1]; // stamp key
    for (auto k_it = collectd_keys.begin(); k_it != collectd_keys.end(); k_it++)
    {
      string collectd_key = k_it->second;
      string visgoth_key = k_it->first;
      if (list_key.find(collectd_key) != string::npos)
      {
        vector<string> glines = collectd.get(list_key);
        for (auto g_it = glines.begin(); g_it != glines.end(); g_it++)
        {
          vector<string> vals = split(*g_it, ' '); // v1=x v2=y
          for (auto v_it = vals.begin(); v_it != vals.end(); v_it++)
          {
            vector<string> value_vec = split(*v_it, '='); // v1=x
            string value = value_vec[0];
            float float_val = stof(value_vec[1]);
            float_values[visgoth_key + "-" + value] += float_val;
          }
        }
      }
    }
  }
  return Json(float_values);
}

uint Visgoth::get_extent(Json profile_data)
{
  cout << get_collectd_stats().dump() << endl;
  return profile_data["extent"].int_value();
}
