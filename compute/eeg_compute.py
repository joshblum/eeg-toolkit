import numpy as np
import ctypes
import os
import time

from os.path import dirname as parent

APPROOT = parent(os.path.realpath(__file__))


class EEGSpecParams(ctypes.Structure):
  _fields_ = [
      ('mrn', ctypes.POINTER(ctypes.c_char)),
      ('startTime', ctypes.c_float),
      ('endTime', ctypes.c_float),
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
    spec_params_p,
]
_libspectrogram.print_spec_params_t.restype = ctypes.c_void_p


def print_spec_params_t(spec_params):
  _libspectrogram.print_spec_params_t(spec_params)

# close_edf
_libspectrogram.close_edf.argtypes = [
    ctypes.POINTER(ctypes.c_char),
]
_libspectrogram.close_edf.restype = ctypes.c_void_p


def close_edf(mrn):
  _libspectrogram.close_edf(mrn)

# get_eeg_spectrogram_params
_libspectrogram.get_eeg_spectrogram_params.argtypes = [
    spec_params_p,
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_float,
    ctypes.c_float,
]
_libspectrogram.get_eeg_spectrogram_params.restype = ctypes.c_void_p


def get_eeg_spectrogram_params(mrn, start_time, end_time):
  spec_params = EEGSpecParams()
  _libspectrogram.get_eeg_spectrogram_params(spec_params,
                                             mrn, start_time, end_time)
  print_spec_params_t(spec_params)
  return spec_params


# eeg_spectrogram_as_arr
_libspectrogram.eeg_spectrogram_as_arr.argtypes = [
    spec_params_p,
    ctypes.c_int,
    np.ctypeslib.ndpointer(dtype=np.float32),
]
_libspectrogram.eeg_spectrogram_as_arr.restype = ctypes.c_void_p


def eeg_spectrogram_as_arr(spec_params, ch):
  spec_arr = np.zeros(
      (spec_params.nblocks, spec_params.nfreqs), dtype=np.float32)
  spec_arr = np.asarray(spec_arr)
  _libspectrogram.eeg_spectrogram_as_arr(spec_params, ch, spec_arr)
  return spec_arr

# mrn_to_filename
_libspectrogram.mrn_to_filename.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.POINTER(ctypes.c_char),
]
_libspectrogram.mrn_to_filename.restype = ctypes.c_void_p


def mrn_to_filename(mrn):
  filename = ""
  _libspectrogram.mrn_to_filename(mrn, filename)
  return filename

# get_change_points
_libspectrogram.example_change_points_as_arr.argtypes = [
    np.ctypeslib.ndpointer(dtype=np.float32),
    ctypes.c_int,
    ctypes.c_int,
]
_libspectrogram.example_change_points_as_arr.restype = ctypes.c_void_p


def example_change_points_as_arr(spec_mat):
  spec_mat = spec_mat.astype('float32')
  n_rows, n_cols = spec_mat.shape
  _libspectrogram.example_change_points_as_arr(spec_mat, n_rows, n_cols)


def main(mrn, start_time, end_time):
  spec_params = get_eeg_spectrogram_params(mrn, start_time, end_time)
  start = time.time()
  spec_mat = eeg_spectrogram_as_arr(spec_params, 0)  # channel LL
  end = time.time()
  print 'Total time: ', (end - start)
  print 'Spectrogram shape:', str(spec_mat.shape)
  print 'Sample data:', spec_mat[:10, :10]
  close_edf(mrn)

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-mrn', '--medical-record-number',
                      default='007',
                      dest='mrn',
                      help='medical record number for spectrogram data.')
  parser.add_argument('-s', '--start_time', default=0.0, type=float,
                      dest='start_time', help='start time of the data')
  parser.add_argument('-e', '--end_time', default=4.0, type=float,
                      dest='end_time', help='end time of the data')
  args = parser.parse_args()
  main(args.mrn, args.start_time, args.end_time)
