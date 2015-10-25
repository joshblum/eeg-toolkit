import struct
import platform

import numpy as np
# import h5py

from eegtools.io import load_edf
from constants import CHANNEL_INDEX

DOWNSAMPLE_RATE = 10
MAX_SIZE = 10 * 100 ** 3  # arbitrarily 10 MB limit
TILEDB = 'tiledb'
SCIDB = 'scidb'


def from_bytes(b):
  return struct.unpack('@i', b)[0]


def to_bytes(n):
  return struct.pack('@i', n)


def grouper(iterable, n, nfft):
  if len(iterable) < n + nfft:
    yield iterable
  else:
    for i in xrange(nfft, len(iterable) - n - nfft, n):
      yield iterable[i - nfft:i + n + nfft]
    # TODO(joshblum): This doesn't seem right..
    yield iterable[i + n - nfft:]


def downsample(spectrogram, n=DOWNSAMPLE_RATE, phase=0):
  """
      Decrease sampling rate by intger factor n with included offset phase
      if the nbytes of the spectrogram exceeds MAX_SIZE
  """
  if spectrogram.nbytes > MAX_SIZE:
    spectrogram = spectrogram[phase::n]
  return spectrogram


def downsample_extent(nblocks, nfreqs, n=DOWNSAMPLE_RATE):
  # calculate the number of bytes
  if nblocks * nfreqs * 4 > MAX_SIZE:
    nblocks = nblocks / DOWNSAMPLE_RATE
  return int(nblocks), int(nfreqs)


def astype(ndarray, _type='float32'):
  """
      Helper to chagne the type of a numpy array.
      the `float32` type is necessary for correct parsing on the client
  """
  return ndarray.astype(_type)  # nececssary for type for diplay


def load_h5py_spectrofile(filename):
  raise NotImplementedError
#  f = h5py.File(filename, 'r')
#  data = f['data']
#  fs = f['Fs'][0][0]
#  return data, fs


def load_edf_spectrofile(filename):
  edf = load_edf(filename)

  fs = edf.sample_rate
  # convert the edf channels to match the index
  data = [None] * len(CHANNEL_INDEX)
  for edf_idx, ch in enumerate(edf.chan_lab):
    spec_idx = CHANNEL_INDEX.get(ch.lower())
    if spec_idx is not None:
      data[spec_idx] = edf.X[edf_idx, :]

  return np.array(data).T, fs


def load_spectrofile(filename):
  spectrofile_map = {
      'eeg': load_h5py_spectrofile,
      'edf': load_edf_spectrofile,
  }

  try:
    file_ext = filename.split('.')[-1]
  except IndexError:
    file_ext = ''

  return spectrofile_map.get(file_ext, load_h5py_spectrofile)(filename)


def mrn_to_filename(mrn):
  """
      Returns a filename containing the data associated with `mrn` This
      should only be used for temporary testing before a real backend is
      implemented.
  """
  basedir = ''
  system = platform.system()
  if system == 'Linux':
    basedir = '/home/ubuntu'
  elif system == 'Darwin':
    basedir = '/Users/joshblum/Dropbox (MIT)'
  return '%(basedir)s/MIT-EDFs/MIT-CSAIL-%(mrn)s.edf' % {
      'basedir': basedir,
      'mrn': mrn,
  }


def get_array_data(mrn, backend=None):
  """
  Converts a medical record number (mrn) to array data for the specified
  backend. Supported backends could be `None` (file), `tiledb`, or `scidb`.
  """
  if backend is None:
    filename = mrn_to_filename(mrn)
    return load_spectrofile(filename)

  elif backend == TILEDB:
    raise NotImplementedError
  elif backend == SCIDB:
    raise NotImplementedError
  else:
    raise Exception('Unknown backend %s' % backend)
