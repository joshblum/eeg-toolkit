#include "visgoth.hpp"

#include <iostream>
#include <string>
#include <vector>
#include "sys/socket.h"
#include "collectd.hpp"

#include "../json11/json11.hpp"
#include "../helpers.hpp"

using namespace std;
using namespace json11;


Visgoth::Visgoth()
{
  //vector<string> lines = collectd.get("eeg-toolkit-dev.csail.mit.edu/users/users");
  //collectd.get("eeg-toolkit-dev.csail.mit.edu/users/users");
  vector<string> lines = collectd.list();
  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    cout << *it << endl;
    vector<string> val = split(*it, ' ');
    string stamp = val[0];
    string key = val[1];
    vector<string> glines = collectd.get(key);
    cout << stamp << " " << key << " ";
    for (vector<string>::iterator it1 = glines.begin(); it1 != glines.end(); it1++)
    {
      cout << *it1 << ", ";
    }
    cout << endl;
  }
}

uint Visgoth::get_extent(Json profile_data)
{
  return profile_data["extent"].int_value();
}
