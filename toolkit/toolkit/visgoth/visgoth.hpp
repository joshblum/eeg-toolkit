#ifndef VISGOTH_H
#define VISGOTH_H

#include <string>
#include "../json11/json11.hpp"

using namespace std;
using namespace json11;

class Visgoth
{
  public:
    uint get_extent(Json profile_data);
};

#endif // VISGOTH_H

