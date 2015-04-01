import struct


DOWNSAMPLE_RATE = 10
MAX_SIZE = 10 * 100**3  # arbitrarily 10 MB limit


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


def astype(ndarray, _type='float64'):
  """
      Helper to chagne the type of a numpy array.
      the `float64` type is necessary for correct parsing on the client
  """
  return ndarray.astype(_type)  # nececssary for type for diplay
