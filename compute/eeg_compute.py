import numpy as np
import ctypes
import os
import time

from os.path import dirname as parent

APPROOT = os.path.join(parent(os.path.realpath(__file__)), 'eeg-compute')


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


# eeg_spectrogram_handler
_libspectrogram.eeg_spectrogram_handler.argtypes = [
    spec_params_p,
    ctypes.c_int,
    np.ctypeslib.ndpointer(dtype=np.float32),
]
_libspectrogram.eeg_spectrogram_handler.restype = ctypes.c_void_p


def eeg_spectrogram_handler(spec_params, ch):
  out = np.zeros((spec_params.nblocks, spec_params.nfreqs), dtype=np.float32)
  out = np.asarray(out)
  _libspectrogram.eeg_spectrogram_handler(spec_params, ch, out)
  return out


def main(filename, duration):
  spec_params = get_eeg_spectrogram_params(filename, duration)
  start = time.time()
  spec = eeg_spectrogram_handler(spec_params, 0)  # channel LL
  end = time.time()
  print 'Total time: ',  (end - start)
  print 'Spectrogram shape:',  str(spec.shape)
  print 'Sample data:',  spec[:10, :10]

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename', default='/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-001.edf',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0,
                      dest='duration', help='duration of the data')
  args = parser.parse_args()
  main(args.filename, args.duration)
