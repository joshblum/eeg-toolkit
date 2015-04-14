#include <stdio.h>
#include <stdlib.h>

#include "edflib.h"
#include "eeg_spectrogram.h"

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
    printf("Using filename: %s, duration: %.2f hours\n", filename, duration);
    unsigned long long start = getticks();
    eeg_file_spectrogram_handler(filename, duration, LL, NULL);
    unsigned long long end = getticks();
    log_time_diff(end - start);
  } else {
    printf("\nusage: spectrogram <filename> <duration>\n\n");
  }
  return 1;
}
