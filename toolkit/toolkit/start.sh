#!/bin/bash

make clean
make edf_converter
make precompute_spectrogram
python file_watcher.py&
make ws_server
./ws_server&
