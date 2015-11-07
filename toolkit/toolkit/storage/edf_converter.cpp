#include "backends.hpp"

void convert_to_array(string mrn, string backend_name) {
  if (backend_name == "HDF5Backend")
  {
    HDF5Backend hdf5_backend;
    hdf5_backend.edf_to_array(mrn);
  }
  else if (backend_name == "TileDBBackend")
  {
    TileDBBackend tiledb_backend;
    tiledb_backend.edf_to_array(mrn);
  }
  else
  {
    cout << "Unknown backend: " << backend_name << endl;
  }
}

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

    string backend_name;
    if (argc == 3)
    {
      backend_name = argv[2];
    }
    else {
      // default backend_name
      backend_name = "HDF5Backend";
    }
    cout << "Using mrn: " << mrn << " and backend: " << backend_name << endl;
    convert_to_array(mrn, backend_name);
  }
  else
  {
    cout << "\nusage: main <mrn> <backend_name>\n" << endl;
  }
  return 1;
}

