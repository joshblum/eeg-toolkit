import numpy as np
import ctypes


_libspectrogram = np.ctypeslib.load_library('libspectrogram', '.')

_libspectrogram.eeg_file_spectrogram_handler.argtypes = [
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_float,
    np.ctypeslib.ndpointer(dtype=np.float),
]
_libspectrogram.eeg_file_spectrogram_handler.restype = ctypes.c_void_p


def eeg_file_spectrogram_handler(filename, duration, out):
  out = np.asarray(out, dtype=np.float)
  _libspectrogram.eeg_file_spectrogram_handler(filename, duration, out)
  return out

if __name__ == '__main__':
  import argparse
  import time

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename', default='/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-001.edf',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0,
                      dest='duration', help='duration of the data')
  # todo get args and build properly size out array first
  args = parser.parse_args()
  start = time.time()
  out = np.zeros((512, 353), dtype=np.float32)
  specs = eeg_file_spectrogram_handler(args.filename, args.duration, out)
  end = time.time()
  print 'Total time: ',  (end - start)
  print 'Spectrogram shape:',  str(specs.shape)
  print 'Sample data:',  specs[:10, :10]
