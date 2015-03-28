import numpy as np
import ctypes


_libspectrogram = np.ctypeslib.load_library('libspectrogram', '.')

_libspectrogram.eeg_file_spectrogram.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_float,
    np.ctypeslib.ndpointer(dtype=np.float),
]
_libspectrogram.eeg_file_spectrogram.restype = ctypes.c_void_p


def eeg_file_spectrogram(filename, duration, out):
  out = np.asarray(out, dtype=np.float)
  _libspectrogram.eeg_file_spectrogram(filename, duration, out)
  return out

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename', default='/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-001.edf',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0,
                      dest='duration', help='duration of the data')

  args = parser.parse_args()
  out = np.zeros((512, 512), dtype=np.float32)

  eeg_file_spectrogram(args.filename, args.duration, out)
