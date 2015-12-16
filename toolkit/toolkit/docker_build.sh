#!/bin/bash

make clean
make edf_converter -j4
make precompute_spectrogram -j4
make ws_server -j4
