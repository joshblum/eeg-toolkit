#ifndef VISGOTH_H
#define VISGOTH_H

#include <string>
#include "../json11/json11.hpp"

using namespace std;
using namespace json11;

class Visgoth
{
  private:
    Json stats;

  public:

    // Constructor
    Visgoth(Json stats)
    {
      this->stats = stats;
    }

    //TODO(xxx): define interfaces for visgoth object
};

#endif // VISGOTH_H

