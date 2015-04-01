import h5py
import operator
import math
import time

import numpy as np
from collections import namedtuple
from scipy import signal
from spectrum import dpss

from helpers import grouper


LL = 0
LP = 1
RP = 2
RL = 3
CHANNELS = [LL, LP, RP, RL]
DIFFERENCE_PAIRS = {
    LL: [
        ('fp1', 'f7'),
        ('f7', 't3'),
        ('t3', 't5'),
        ('t5', 'o1'),
    ],
    LP: [
        ('fp1', 'f3'),
        ('f3', 'c3'),
        ('c3', 'p3'),
        ('p3', 'o1'),
    ],
    RP: [
        ('fp2', 'f4'),
        ('f4', 'c4'),
        ('c4', 'p4'),
        ('p4', 'o2'),
    ],
    RL: [
        ('fp2', 'f8'),
        ('f8', 't4'),
        ('t4', 't6'),
        ('t6', 'o2'),
    ],
}

CHANNEL_INDEX = {
    'fp1': 0,
    'f3': 1,
    'c3': 2,
    'p3': 3,
    'o1': 4,
    'fp2': 5,
    'f4': 6,
    'c4': 7,
    'p4': 8,
    'o2': 9,
    'f7': 10,
    't3': 11,
    't5': 12,
    'f8': 13,
    't4': 14,
    't6': 15,
    'fz': 16,
    'cz': 17,
    'pz': 18,
}

CHUNK_HOURS = 1.0

EEGSpecParams = namedtuple(
    'SpecParams', ['chunksize', 'fs',
                   'shift', 'spec_len',
                   'Nstep', 'trial_avg',
                   'nfft', 'nblocks', 'tapers',
                   'nfreqs', 'nsamples', 'findx'])
AudioSpecParams = namedtuple(
    'SpecParams', ['chunksize', 'fs',
                   'shift', 'spec_len',
                   'nfft', 'nblocks',
                   'nfreqs', 'nsamples'])

# see if we are running normally or using kernprof
try:
  profile(lambda x: None)
except NameError:
  def noop_decorator(fn):
    return fn
  profile = noop_decorator


def _get_nsamples(data, fs, duration):
  """
    Determine the number of samples to take
    from a data source based on the duration (hrs)
    specified
  """
  data_len = len(data)
  if duration is None:
    return data_len
  nsamples = min(data_len, int(fs * 60 * 60 * duration))
  print "Num samples:", nsamples
  return nsamples


def _get_chunksize(nsamples, fs, nfft, num_hours=CHUNK_HOURS):
  """
      A chunk will be at most 1 hr in length.
  """
  chunksize = int(fs * 60 * 60 * num_hours)
  return min(chunksize + (chunksize % nfft), nsamples)


def _get_shift(nfft, overlap):
  return int(round(nfft * overlap))


def _get_nblocks(nsamples, shift, Nstep):
  return int(math.ceil((nsamples - shift) / Nstep)) + 1


def _get_nfreqs(nfft):
  return nfft / 2 + 1


def _hann(n):
  return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(n) / (n - 1))


@profile
def _power_log(x):
  return 2**(math.ceil(math.log(x, 2)))


@profile
def _change_row_to_column(data):
  """
     Transform 1d array into column vectors
  """
  shape = data.shape
  if len(shape) == 1:
    data = np.reshape(data, (-1, shape[0]))
  N, Ch = data.shape
  if (N == 1):
    data = data.T
  return data


def _getfgrid(fs, nfft, fpass):
  df = fs / nfft
  f = np.arange(0, fs, df)
  if f[-1] != fs:
    f = np.append(f, fs)
  f = f[:nfft]
  if len(fpass) != 1:
    findx = (np.where((f >= fpass[0]) & (f <= fpass[1])))
  else:
    min_index, min_value = min(
        enumerate(f - fpass[2]), key=operator.itemgetter(1))
    f = f[min_index]
    findx = min_index
  fout = f[findx]
  return fout, findx[0]


@profile
def _dpsschk(tapers, N, fs):
  """
  Helper function to calculate tapers and
  if precalculated tapers are provided,
  to check that they are the of same
  length in time as the time series being studied.
  """
  tapers, eigs = dpss(N, tapers[0], tapers[1])
  tapers = np.multiply(tapers, np.sqrt(fs))
  return tapers


@profile
def _mtfftc(data, tapers, nfft, fs):
  """ Helper function which calculates the fft of the data using the tapers"""
  NC, C = data.shape
  NK, K = tapers.shape
  if NK != NC:
    print "length of tapers is not compatible with length of data!!"
  # to create the matrix which has n rows X K cols
  data = np.tile(data, (1, K))
  data_proj = np.multiply(data, tapers)
  J = np.fft.fft(data_proj, n=nfft, axis=0) / fs
  return J


def load_h5py_spectrofile(filename):
  f = h5py.File(filename, 'r')
  data = f['data']
  fs = f['Fs'][0][0]
  return data, fs


def get_eeg_spectrogram_params(data, fs, duration, pad=0, fpass=None,
                               trial_avg=False, moving_win=None, tapers=None):

  if fpass is None:
    fpass = [0, 55]
  if moving_win is None:
    moving_win = [4, 1]
  if tapers is None:
    tapers = [3, 5]

  shift = int(round(fs * moving_win[0]))
  Nstep = int(round(fs * moving_win[1]))
  nfft = int(max(_power_log(shift) + pad, shift))

  nsamples = _get_nsamples(data, fs, duration)
  chunksize = _get_chunksize(nsamples, fs, nfft)

  sfreqs, findx = _getfgrid(fs, nfft, fpass)
  nfreqs = len(sfreqs)
  nblocks = _get_nblocks(nsamples, shift, Nstep)

  return EEGSpecParams(fs=fs, shift=shift,
                       Nstep=Nstep, nfft=nfft, findx=findx,
                       nfreqs=nfreqs, nblocks=nblocks,
                       nsamples=nsamples, tapers=tapers,
                       trial_avg=trial_avg,
                       spec_len=int(nsamples / fs),
                       chunksize=chunksize)


@profile
def eeg_ch_spectrogram(ch, data, spec_params, progress_fn=None):
  """
    Compute the spectrogram for an individual spectrogram in the eeg
  """
  T = []
  t0 = time.time()
  # TODO (joshblum): do this once in a preprocessing step.
  pairs = DIFFERENCE_PAIRS.get(ch)
  for i, pair in enumerate(pairs):
    c1, c2 = pair
    # take differences between the channels in each of the regions
    diff = data[:, CHANNEL_INDEX.get(c2)] - data[:, CHANNEL_INDEX.get(c1)]
    T.append(multitaper_spectrogram(
        diff, spec_params))
    if progress_fn:
      progress_fn((i + 1) / len(pairs), canvas_id=ch)

  # compute the regional average of the spectrograms for each channel
  res = sum(T) / 4
  t1 = time.time()
  print "Time for ch %s: %s" % (ch, t1 - t0)
  return res


@profile
def multitaper_spectrogram(data, spec_params):

  data = _change_row_to_column(data)

  fs = spec_params.fs
  nfft = spec_params.nfft

  Nstep = spec_params.Nstep
  shift = spec_params.shift

  nblocks = _get_nblocks(len(data), shift, Nstep)
  nfreqs = spec_params.nfreqs

  if spec_params.trial_avg:
    Ch = data.shape[1]
    S = np.zeros((nblocks, nfreqs, Ch))
  else:
    S = np.zeros((nblocks, nfreqs))
  for idx in xrange(nblocks):
    datawin = signal.detrend(
        data[idx * Nstep:idx * Nstep + shift], type == "constant")
    if idx < 2:
      N = len(datawin)
      taps = _dpsschk(spec_params.tapers, N, fs)
    J = _mtfftc(datawin, taps, nfft, fs)[
        spec_params.findx, :]
    s = np.mean((np.multiply(np.conj(J), J)), axis=1).squeeze()
    if spec_params.trial_avg:
      s = np.mean(s, axis=1).squeeze()
    S[idx, :] = s

  spect = S.squeeze()
  return spect


@profile
def on_eeg_file_spectrogram_profile(filename, duration):

  data, fs = load_h5py_spectrofile(filename)
  spec_params = get_eeg_spectrogram_params(data, fs, duration)

  data = data[:spec_params.nsamples]  # ok lets just chunk a bit of this mess
  print 'loaded file'

  t0 = time.time()
  for chunk in grouper(data, spec_params.chunksize, spec_params.shift):
    chunk = np.array(chunk)
    for ch in CHANNELS:
      print 'computing channel %s' % ch
      spec = eeg_ch_spectrogram(ch, chunk, spec_params)
  t1 = time.time()
  print "Total time: %s" % (t1 - t0)


def get_audio_spectrogram_params(data, fs, duration, nfft, overlap):
  shift = _get_shift(nfft, overlap)
  nsamples = _get_nsamples(data, fs, duration)
  chunksize = _get_chunksize(nsamples, fs, nfft)
  nblocks = _get_nblocks(nsamples, nfft, shift)
  nfreqs = _get_nfreqs(nfft)
  return AudioSpecParams(nfft=nfft, shift=shift,
                         nsamples=nsamples,
                         spec_len=nsamples / fs,
                         nblocks=nblocks, nfreqs=nfreqs,
                         fs=fs, chunksize=chunksize)


def spectrogram(data, spec_params, canvas_id=None, progress_fn=None):
  """Calculate a real spectrogram from audio data

  An audio data will be cut up into overlapping blocks of length
  `nfft`. The amount of overlap will be `overlap*nfft`. Then,
  calculate a real fourier transform of length `nfft` of every
  block and save the absolute spectrum.

  Arguments:
  data      audio data as a numpy array.
  nfft      the FFT length used for calculating the spectrogram.
  shift   the amount of overlap between consecutive spectra.

  """
  nfft = spec_params.nfft
  shift = spec_params.shift
  nblocks = _get_nblocks(len(data), nfft, shift)
  nfreqs = _get_nfreqs(nfft)

  window = _hann(nfft)
  specs = np.zeros((nfreqs, nblocks), dtype=np.float64)
  for idx in xrange(nblocks):
    specs[:, idx] = np.abs(np.fft.rfft(
        data[idx * shift:idx * shift + nfft] * window, n=nfft)) / nfft
    if progress_fn and idx % 10 == 0:
      progress_fn(idx / nblocks, canvas_id=canvas_id)
  specs[:, -1] = np.abs(
      np.fft.rfft(data[nblocks * shift:], n=nfft)) / nfft
  if progress_fn:
    progress_fn(1, canvas_id=canvas_id)
  return specs.T

if __name__ == '__main__':
  import argparse

  parser = argparse.ArgumentParser(
      description='Profile spectrogram code.')
  parser.add_argument('-f', '--filename', default='test.eeg',
                      dest='filename', help='filename for spectrogram data.')
  parser.add_argument('-d', '--duration', default=4.0,
                      dest='duration', help='duration of the data')

  args = parser.parse_args()

  on_eeg_file_spectrogram_profile(args.filename, args.duration)
