#!/bin/bash
set -x #echo on

# Runner script for ingesting files of different files and sizes for different
# READ_CHUNK_SIZE.

WORK_DIR=".."
cd $WORK_DIR

MRN="005"
RESULTS_FILE="experiments/ingestion_results.txt"

NUM_RUNS=3
BACKENDS="BinaryBackend HDF5Backend"
# use TileDB when metadata is implemented
# TileDBBackend"
READ_CHUNK_SIZES="1 2 4 8 16 32 64 128 256 512" #MB
FILE_SIZES="1 2 4 8 16 32 64 128" # GB

# setup symlinks for file names
for file_size in $FILE_SIZES; do
  ln -s ~/eeg-data/eeg-data/${MRN}.edf ~/eeg-data/eeg-data/${MRN}-${file_size}gb.edf
done;

for i in $(seq 1 ${NUM_RUNS}); do
  for backend in $BACKENDS; do
    for file_size in $FILE_SIZES; do
      for read_chunk in $READ_CHUNK_SIZES; do
        make clean;
        make edf_converter READ_CHUNK=$read_chunk;
        # edf_converter <mrn> <backend> <desired_size>
        ./edf_converter ${MRN}-${file_size}gb $backend $file_size | tee -a $RESULTS_FILE
      done;
    done;
  done;
done;

