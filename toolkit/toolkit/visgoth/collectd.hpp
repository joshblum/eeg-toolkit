#ifndef COLLECTD_H
#define COLLECTD_H

#include <string>
#include "../config.hpp"
#include "../json11/json11.hpp"

using namespace std;
using namespace json11;

class Collectd
{
  private:
    const char* socket_path = COLLECTD_SOCK;
    int fd;
    uint _cmd(string cmd);
    string _readline();
    vector<string> _readlines(uint sizehint);


  public:
    Collectd();
    ~Collectd();
    vector<string> list();
    vector<string> get(string val, bool flush=true);
};

#endif // COLLECTD_H

