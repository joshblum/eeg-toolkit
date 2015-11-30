#include <string>
#include <armadillo>
#include <math.h>

#include "eeg_spectrogram.hpp"

using namespace std;

/*
 * Command line program to convert a given `mrn` to precalculated spectrograms
 * uses the current backend defined in `config.hpp`
 */
int main(int argc, char* argv[])
{
  if (argc <= 2)
  {
    string mrn;
    if (argc == 2)
    {
      mrn = argv[1];
    }
    else
    {
      // default medial record number
      mrn = "007";
    }

    cout << "Using mrn: " << mrn << " backend: " << TOSTRING(BACKEND) <<" and WRITE_CHUNK_SIZE: " << WRITE_CHUNK_SIZE << endl;
    StorageBackend backend;
    precompute_spectrogram(mrn, &backend);
  }
  else
  {
    cout << "\nusage: ./precompute_spectrogram <mrn>\n" << endl;
  }
  return 1;
}
