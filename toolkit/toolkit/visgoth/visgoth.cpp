#include "visgoth.hpp"

#include <string>
#include "../json11/json11.hpp"

using namespace std;
using namespace json11;


uint Visgoth::get_extent(Json profile_data)
{
  return profile_data["extent"].int_value();
}
