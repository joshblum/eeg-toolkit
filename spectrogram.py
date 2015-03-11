import h5py
import math
import time

import numpy as np
from collections import namedtuple
SpecParams = namedtuple(
    'SpecParams', ['nfft', 'shift',
                   'nblocks', 'nfreqs',
                   'spec_len', 'fs', 'nsamples'])

CHANNELS = ['LL', 'LP', 'RP', 'RL']
DIFFERENCE_PAIRS = {
    'LL': [
        ('fp1', 'f7'),
        ('f7', 't3'),
        ('t3', 't5'),
        ('t5', 'o1'),
    ],
    'LP': [
        ('fp1', 'f3'),
        ('f3', 'c3'),
        ('c3', 'p3'),
        ('p3', 'o1'),
    ],
    'RP': [
        ('fp2', 'f4'),
        ('f4', 'c4'),
        ('c4', 'p4'),
        ('p4', 'o2'),
    ],
    'RL': [
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


def _hann(n):
  return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(n) / (n - 1))


def _power_log(x):
  return 2**(math.ceil(math.log(x, 2)))


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


def _get_nblocks(data, nfft, shift):
  return int((len(data) - nfft) / shift + 1)


def _get_nfreqs(nfft):
  return nfft / 2 + 1


def load_h5py_spectrofile(filename):
  f = h5py.File(filename, 'r')
  data = f['data']
  fs = f['Fs'][0][0]
  return data, fs


def get_eeg_spectrogram_params(data, duration, nfft, overlap, fs):
  """
      Emulating the original matlab code with these
      hardcoded constants for the window parameters
  """
  # TODO (joshblum): Need to take nfft into account instead of hardcoding
  pad = 0
  Nwin = int(fs * 1.5)
  Nstep = int(fs * 0.2)
  nfft = int(max(_power_log(Nwin) + pad, Nwin))
  shift = round(nfft * overlap)
  nsamples = _get_nsamples(data, fs, duration)
  nblocks = _get_nblocks(data, nfft, shift)
  nfreqs = _get_nfreqs(nfft)
  return SpecParams(nfft=nfft, shift=Nstep,
                    nsamples=nsamples,
                    spec_len=nsamples / fs,
                    nblocks=nblocks, nfreqs=nfreqs, fs=fs)


def get_audio_spectrogram_params(data, duration, nfft, overlap, fs):
  shift = round(nfft * overlap)
  nsamples = _get_nsamples(data, fs, duration)
  nblocks = _get_nblocks(data, nfft, shift)
  nfreqs = _get_nfreqs(nfft)
  return SpecParams(nfft=nfft, shift=shift,
                    nsamples=nsamples,
                    spec_len=nsamples / fs,
                    nblocks=nblocks, nfreqs=nfreqs, fs=fs)


def eeg_ch_spectrogram(ch, data, spec_params, progress_fn):
  """
    Compute the spectrogram for an individual spectrogram in the eeg
  """
  T = []
  t0 = time.time()
  pairs = DIFFERENCE_PAIRS.get(ch)
  for i, pair in enumerate(pairs):
    c1, c2 = pair
    # take differences between the channels in each of the regions
    diff = data[:, CHANNEL_INDEX.get(c2)] - data[:, CHANNEL_INDEX.get(c1)]
    T.append(spectrogram(diff, spec_params, canvas_id=ch))
    progress_fn((i + 1) / len(pairs))
    # compute the regional average of the spectrograms for each channel
  res = sum(T) / 4
  t1 = time.time()
  print "Time for ch %s: %s" % (ch, t1 - t0)
  return res


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
  nblocks = spec_params.nblocks
  nfreqs = spec_params.nfreqs

  window = _hann(nfft)
  specs = np.zeros((nfreqs, nblocks), dtype=np.float32)
  for idx in xrange(nblocks):
    specs[:, idx] = np.abs(np.fft.rfft(
        data[idx * shift:idx * shift + nfft] * window, n=nfft)) / nfft
    if progress_fn and idx % 10 == 0:
      progress_fn(idx / nblocks)
  specs[:, -1] = np.abs(
      np.fft.rfft(data[nblocks * shift:], n=nfft)) / nfft
  if progress_fn:
    progress_fn(1)
  return specs.T
