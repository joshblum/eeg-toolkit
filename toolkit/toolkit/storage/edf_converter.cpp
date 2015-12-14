#include <string>
#include <fstream>
#include "backends.hpp"
#include "../helpers.hpp"

using namespace std;

size_t gigabytes_to_bytes(size_t num_gb)
{
  return num_gb * 1000000000;
}

/*
 * Command line program to convert an
 * EDF file for the given `mrn` to the specified backend
 */
int main(int argc, char* argv[])
{
  if (argc <= 3)
  {
    string mrn;
    if (argc >= 2)
    {
      mrn = argv[1];
    }
    else
    {
      // default medial record number
      mrn = "007";
    }

    size_t desired_size;
    if (argc == 3)
    {
      desired_size = atoi(argv[2]);
    }
    else
    {
      desired_size = 0;
    }

    desired_size = gigabytes_to_bytes(desired_size);

    cout << "Using mrn: " << mrn << " backend: " << TOSTRING(BACKEND) << " with desired_size: " << desired_size << " and READ_CHUNK_SIZE: " << READ_CHUNK_SIZE << endl;
    StorageBackend backend;
    edf_to_array(mrn, &backend, desired_size);
  }
  else
  {
    cout << "\nusage: ./edf_converter <mrn> <desired_size>\n" << endl;
  }
  return 1;
}

