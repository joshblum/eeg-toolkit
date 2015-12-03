#include "collectd.hpp"

#include <errno.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../json11/json11.hpp"
#include "../helpers.hpp"

using namespace std;
using namespace json11;


Collectd::Collectd()
{
  struct sockaddr_un addr;
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    cout << "Socket error: " << errno << endl;
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    cout << "Connect error: " << errno << endl;
    exit(-1);
  }
}

Collectd::~Collectd()
{
  close(fd);
}

vector<string> Collectd::list()
{
  uint numvalues = _cmd("LISTVAL");
  return _readlines(numvalues);
}

vector<string> Collectd::get(string val, bool flush)
{
  uint numvalues = _cmd("GETVAL \"" + val + "\"");
  vector <string> lines = _readlines(numvalues);
  if (flush)
  {
    _cmd("FLUSH identifier=\"" + val + "\"");
  }
  return lines;
}

uint Collectd::_cmd(string cmd)
{
  cmd += "\n";
  if (write(fd, cmd.c_str(), cmd.size()) < 0)
  {
    cout << "Write error: " << errno  << endl;
    exit(-1);
  }
  string line = _readline();
  if (!line.size()) {
    return 0;
  } else {
    vector<string> stat = split(line, ' ');
    return stoi(stat[0]);
  }
}

string Collectd::_readline()
{
  string line = "";
  char data = ' ';
  while (data != '\n')
  {
    read(fd, &data, sizeof(data));
    if (data != '\n')
    {
      line += data;
    }
  }
  return line;
}

vector<string> Collectd::_readlines(uint nlines)
{
  vector<string> lines;
  while (1)
  {
    string line = _readline();
    if (!line.size())
    {
      break;
    }
    lines.push_back(line);
    if (nlines && lines.size() >= nlines)
    {
      break;
    }
  }
  return lines;
}

