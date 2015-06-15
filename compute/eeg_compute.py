import numpy as np
import ctypes
import os
import time

from os.path import dirname as parent
from sys import platform as _platform

APPROOT = parent(os.path.realpath(__file__))


class EEGSpecParams(ctypes.Structure):
  _fields_ = [
      ('filename', ctypes.POINTER(ctypes.c_char)),
      ('duration', ctypes.c_float),
      ('hdl', ctypes.c_int),
      ('spec_len', ctypes.c_int),
      ('fs', ctypes.c_int),
      ('nfft', ctypes.c_int),
      ('nstep', ctypes.c_int),
      ('shift', ctypes.c_int),
      ('nsamples', ctypes.c_int),
      ('nblocks', ctypes.c_int),
      ('nfreqs', ctypes.c_int),
  ]

spec_params_p = ctypes.POINTER(EEGSpecParams)

# load shared library
_libspectrogram = np.ctypeslib.load_library('lib_eeg_spectrogram', APPROOT)

# print_spec_params_t
_libspectrogram.print_spec_params_t.argtypes = [
    ctypes.POINTER(EEGSpecParams),
]
_libspectrogram.print_spec_params_t.restype = ctypes.c_void_p


def print_spec_params_t(spec_params):
  _libspectrogram.print_spec_params_t(spec_params)

# close_edf
_libspectrogram.close_edf.argtypes = [
    ctypes.POINTER(ctypes.c_char),
]
_libspectrogram.close_edf.restype = ctypes.c_void_p


def close_edf(filename):
  _libspectrogram.close_edf(filename)

# get_eeg_spectrogram_params
_libspectrogram.get_eeg_spectrogram_params.argtypes = [
    spec_params_p,
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_float,
]
_libspectrogram.get_eeg_spectrogram_params.restype = ctypes.c_void_p


def get_eeg_spectrogram_params(filename, duration):
  spec_params = EEGSpecParams()
  _libspectrogram.get_eeg_spectrogram_params(spec_params,
                                             filename, duration)
  print_spec_params_t(spec_params)
  return spec_params


# eeg_spectrogram_handler_as_arr
_libspectrogram.eeg_spectrogram_handler_as_arr.argtypes = [
    spec_params_p,
    ctypes.c_int,
    np.ctypeslib.ndpointer(dtype=np.float32),
]
_libspectrogram.eeg_spectrogram_handler_as_arr.restype = ctypes.c_void_p


def eeg_spectrogram_handler_as_arr(spec_params, ch):
  spec_arr = np.zeros(
      (spec_params.nblocks, spec_params.nfreqs), dtype=np.float32)
  spec_arr = np.asarray(spec_arr)
  _libspectrogram.eeg_spectrogram_handler_as_arr(spec_params, ch, spec_arr)
  return spec_arr


def main(filename, duration):
  spec_params = get_eeg_spectrogram_params(filename, duration)
  start = time.time()
  spec = eeg_spectrogram_handler_as_arr(spec_params, 0)  # channel LL
  end = time.time()
  print 'Total time: ',  (end - start)
  print 'Spectrogram shape:',  str(spec.shape)
  print 'Sample data:',  spec[:10, :10]
  close_edf(filename)

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename',
                      # default='/home/ubuntu/MIT-EDFs/MIT-CSAIL-007.edf',
                      default='/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-007.edf',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0, type=float,
                      dest='duration', help='duration of the data')
  args = parser.parse_args()
  main(args.filename, args.duration)
