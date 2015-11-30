#!/bin/bash
set -x #echo on

# Runner script for precomputing files of different files and sizes for
# different WRITE_CHUNK_SIZEs.

if [ "$#" -ne 1 ]; then
  echo "usage: precompute_runner [BinaryBackend|HDF5Backend|TileDBBackend]"
  exit
fi

WORK_DIR=".."
cd $WORK_DIR

MRN="005"
RESULTS_FILE="experiments/precompute_results.txt"
mv $RESULTS_FILE $RESULTS_FILE-bak-$(date +%s)

NUM_RUNS=3
BACKEND=$1
WRITE_CHUNK_SIZES="1 2 4 8 16 32 64 128 256 512" #MB
FILE_SIZES="1 2 4 8 16 32 64 128" # GB

for i in $(seq 1 ${NUM_RUNS}); do
  for file_size in $FILE_SIZES; do
    for write_chunk in $WRITE_CHUNK_SIZES; do
    make clean
    make precompute_spectrogram WRITE_CHUNK=$write_chunk BACKEND=$BACKEND
    # precompute_spectrogram <mrn>
    ./precompute_spectrogram ${MRN}-${file_size}gb |& tee -a $RESULTS_FILE
    done;
  done;
done;

