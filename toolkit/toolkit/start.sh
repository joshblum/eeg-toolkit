#!/bin/bash

make clean
make edf_converter -j4
make precompute_spectrogram -j4
python file_watcher.py&
make ws_server -j4
./ws_server&
