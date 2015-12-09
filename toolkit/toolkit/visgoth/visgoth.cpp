#include "visgoth.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "sys/socket.h"
#include "collectd.hpp"

#include "HappyHTTP/happyhttp.h"
#include "../json11/json11.hpp"
#include "../helpers.hpp"

using namespace std;
using namespace json11;

static int BUF_SIZE = 1024;

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

void OnBegin(const happyhttp::Response* r, void* userdata)
{
}

void OnData(const happyhttp::Response* r, void* userdata, const unsigned char* data, int n)
{
  n = min(n, BUF_SIZE);
  strncpy((char*) userdata, reinterpret_cast<const char*>(data), n);
}

void OnComplete(const happyhttp::Response* r, void* userdata)
{
}

uint Visgoth::get_extent(Json profile_data)
{
  Json collectd_data = get_collectd_stats();
  Json request_data = Json::object {
    {"client_profile", profile_data},
    {"server_profile", collectd_data},
  };

  string request = Json(request_data).dump();
  const char* body = request.c_str();
  int body_size = request.size();

  const char* headers[] =
  {
    "Connection", "close",
    "Content-type", "application/json",
    "Accept", "application/json",
    0
  };

  char* response = (char*) malloc(sizeof(char) * 1024);

  happyhttp::Connection conn("127.0.0.1", 5000);
  conn.setcallbacks(OnBegin, OnData, OnComplete, (void*) response);
  conn.request("POST",
      "/visgoth/get_extent",
      headers,
      (const unsigned char*)body,
      body_size);

  while(conn.outstanding())
    conn.pump();

  Json json_response = Json(response);
  free(response);

  return json_response["extent"].int_value();
}
