#!/bin/bash
set -x #echo on

# Runner script for precompute files of different files and sizes for
# different WRITE_CHUNK_SIZEs.

WORK_DIR=".."
cd $WORK_DIR

MRN="005"
RESULTS_FILE="experiments/precompute_results.txt"
mv $RESULTS_FILE $RESULTS_FILE-bak-$(date +%s)

NUM_RUNS=3
BACKENDS="BinaryBackend HDF5Backend"
# use TileDB when metadata is implemented
# TileDBBackend"
WRITE_CHUNK_SIZES="1 2 4 8 16 32 64 128 256 512" #MB
FILE_SIZES="1 2 4 8 16 32 64 128" # GB

for i in $(seq 1 ${NUM_RUNS}); do
  for backend in $BACKENDS; do
    for file_size in $FILE_SIZES; do
      for write_chunk in $WRITE_CHUNK_SIZES; do
      make clean;
      make precompute_spectrogram WRITE_CHUNK=$write_chunk BACKEND=$backend;
      # precompute_spectrogram <mrn> <backend>
      ./precompute_spectrogram ${MRN}-${file_size}gb | tee -a $RESULTS_FILE
      done;
    done;
  done;
done;

