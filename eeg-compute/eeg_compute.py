import numpy as np
import ctypes
import time


class EEGSpecParams(ctypes.Structure):
  _fields_ = [
      ('filename', ctypes.POINTER(ctypes.c_char)),
      ('duration', ctypes.c_float),
      ('hdl', ctypes.c_int),
      ('spec_len', ctypes.c_int),
      ('fs', ctypes.c_int),
      ('nfft', ctypes.c_int),
      ('Nstep', ctypes.c_int),
      ('shift', ctypes.c_int),
      ('nsamples', ctypes.c_int),
      ('nblocks', ctypes.c_int),
      ('nfreqs', ctypes.c_int),
  ]

spec_params_p = ctypes.POINTER(EEGSpecParams)

# load shared library
_libspectrogram = np.ctypeslib.load_library('lib_eeg_spectrogram', '.')

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
  return spec_params


# eeg_file_spectrogram_handler
_libspectrogram.eeg_file_spectrogram_handler.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_float,
    np.ctypeslib.ndpointer(dtype=np.float),
]
_libspectrogram.eeg_file_spectrogram_handler.restype = ctypes.c_void_p


def eeg_file_spectrogram_handler(filename, duration, spec_params):
  out = np.zeros((spec_params.nfreqs, spec_params.nblocks), dtype=np.float64)
  out = np.asarray(out)
  _libspectrogram.eeg_file_spectrogram_handler(filename, duration, out)
  return out


def main(filename, duration):
  spec_params = get_eeg_spectrogram_params(filename, duration)
  start = time.time()
  specs = eeg_file_spectrogram_handler(filename, duration, spec_params)
  end = time.time()
  print 'Total time: ',  (end - start)
  print 'Spectrogram shape:',  str(specs.shape)
  print 'Sample data:',  specs[:10, :10]

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename', default='/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-001.edf',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0,
                      dest='duration', help='duration of the data')
  # todo get args and build properly size out array first
  args = parser.parse_args()
  main(args.filename, args.duration)
