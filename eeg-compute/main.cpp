#include <stdio.h>
#include <stdlib.h>

#include "edflib.h"
#include "spectrogram.h"

int main(int argc, char *argv[])
{
  if (argc >= 2) {
    float duration;
    char* filename = argv[1];
    if (argc == 3) {
      duration = atof(argv[2]);
    } else {
      duration = 4.0; // default duration
    }
    printf("Using filename: %s, duration: %.2f\n", filename, duration);
    return eeg_file_spectrogram(filename, duration);
  } else {
    printf("\nusage: spectrogram <filename> <duration>\n\n");
    return 1;
  }
}
