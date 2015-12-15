#!/bin/bash
set -x #echo on

# Runner script for ingesting files of different files and sizes for different
# READ_CHUNK_SIZE.

if [ "$#" -ne 1 ]; then
  echo "usage: cp_baseline [bin|h5]"
  exit
fi

EXT=$1
EMAIL="joshblum@mit.edu"
HOSTNAME=$(hostname)
EXP_NAME="cp experiment"

WORK_DIR=".."
cd $WORK_DIR

MRN="005"
RESULTS_FILE="experiments/ingestion_results.txt-cp"
mv $RESULTS_FILE $RESULTS_FILE-bak-$(date +%s)

FILE_SIZES="1 2 4 8 16 32 64 128" # GB

for file_size in $FILE_SIZES; do
  sync && sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' 
  start=`date +%s.%N`
  filename=~/eeg-data/eeg-data/${MRN}-${file_size}gb.${EXT}
  cp "${filename}" "${filename}-bak"
  end=`date +%s.%N`
  runtime=$(echo "$end - $start" | bc)
  echo "experiment_data::${MRN}-${file_size}gb,cp,${file_size}000000000,-1,${runtime}" >> $RESULTS_FILE
done;

PAYLOAD="${HOSTNAME} completed ${EXP_NAME} for ${EXT} $(date)"
./experiments/send_mail.sh "$EMAIL" "$PAYLOAD"

