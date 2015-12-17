#!/bin/bash

make clean
make edf_converter _VISGOTH_IP=visgoth -j4
make precompute_spectrogram _VISGOTH_IP=visgoth -j4
make ws_server _VISGOTH_IP=visgoth -j4
