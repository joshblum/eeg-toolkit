from __future__ import division

import io
import json
import h5py
import struct
import math
import numpy as np

from tornado.websocket import WebSocketHandler
from pysoundfile import SoundFile

NUM_SAMPLES = 200 * 60 * 60 * 2  # 200Hz * 60s * 60m = 2hr of data

MAX_SIZE = 100**3  # arbitrarily 1 MB limit


AUDIO = 'audio'
EEG = 'eeg'

CHUNK_SIZE = 4096
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


def power_log(x):
  return 2**(math.ceil(math.log(x, 2)))


def get_spectrogram_params(fs):
  """
      Emulating the original matlab code with these
      hardcoded constants for the window parameters
  """
  # TODO (joshblum): Need to take nfft into account instead of hardcoding
  pad = 0
  Nwin = int(fs * 1.5)
  Nstep = int(fs * 0.2)
  nfft = int(max(power_log(Nwin) + pad, Nwin))
  return nfft, Nstep


def hann(n):
  return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(n) / (n - 1))


def from_bytes(b):
  return struct.unpack('@i', b)[0]


def to_bytes(n):
  return struct.pack('@i', n)


def downsample(spectrogram, n=10, phase=0):
  """
      Decrease sampling rate by intgerfactor n with included offset phase
      if the nbytes of the spectrogram exceeds MAX_SIZE
  """
  if spectrogram.nbytes > MAX_SIZE:
    spectrogram = spectrogram[phase::n]
  return spectrogram


def astype(ndarray, _type='float32'):
  """
      Helper to chagne the type of a numpy array.
      the `float32` type is necessary for correct parsing on the client
  """
  return ndarray.astype(_type)  # nececssary for type for diplay


class JSONWebSocket(WebSocketHandler):

  """A websocket that sends/receives JSON messages.

  Each message has a type, a content and optional binary data.
  Message type and message content are stored as a JSON object. Type
  must be a string and content must be JSON-serializable.

  If binary data is present, the message will be sent in binary,
  with the first four bytes storing a signed integer containing the
  length of the JSON data, then the JSON data, then the binary data.
  The binary data will be stored 8-byte aligned.

  """

  def check_origin(self, origin):
    return True

  def open(self):
    print('WebSocket opened')

  def send_message(self, msg_type, content, data=None):
    """Send a message.

    Arguments:
    msg_type  the message type as string.
    content   the message content as json-serializable data.
    data      raw bytes that are appended to the message.

    """

    if data is None:
      self.write_message(json.dumps({'type': msg_type,
                                     'content': content}).encode())
    else:
      header = json.dumps({'type': msg_type,
                           'content': content}).encode()

      # append enough spaces so that the payload starts at an 8-byte
      # aligned position. The first four bytes will be the length of
      # the header, encoded as a 32 bit signed integer:
      header += b' ' * (8 - ((len(header) + 4) % 8))
      # the length of the header as a binary 32 bit signed integer:
      prefix = to_bytes(len(header))
      self.write_message(prefix + header + data, binary=True)

  def on_message(self, msg):
    """Parses a message

    Each message must contain the message type, the message
    cntent, and an optional binary payload. The decoded message
    will be forwarded to receive_message().

    Arguments:
    msg       the message, either as str or bytes.

    """

    if isinstance(msg, bytes):
      header_len = from_bytes(msg[:4])
      header = msg[4:header_len + 4].decode()
      data = msg[4 + header_len:]
    else:
      header = msg
      data = None

    try:
      header = json.loads(header)
    except ValueError:
      print('message {} is not a valid JSON object'.format(msg))
      return

    if 'type' not in header:
      print('message {} does not have a "type" field'.format(header))
    elif 'content' not in header:
      print('message {} does not have a "content" field'.format(header))
    else:
      self.receive_message(header['type'], header['content'], data)

  def receive_message(self, msg_type, content, data=None):
    """Message dispatcher.

    This is meant to be overwritten by subclasses. By itself, it
    does nothing but complain.

    """

    if msg_type == 'information':
      print(content)
    else:
      print(('Don\'t know what to do with message of type {}' +
             'and content {}').format(msg_type, content))

  def on_close(self):
    print('WebSocket closed')


class SpectrogramWebSocket(JSONWebSocket):

  """A websocket that sends spectrogram data.

  It calculates a spectrogram with a given FFT length and overlap
  for a requested file. The file can either be supplied as a binary
  data blob, or as a file name.

  This implements two message types:
  - request_file_spectrogram, which needs a filename, and optionally
    `nfft` and `overlap`.
  - request_data_spectrogram, which needs the file as a binary data
    blob, and optionally `nfft` and `overlap`.

  """

  def receive_message(self, msg_type, content, data=None):
    """Message dispatcher.

    Dispatches
    - `request_file_spectrogram` to self.on_file_spectrogram
    - `request_data_spectrogram` to self.on_data_spectrogram

    Arguments:
    msg_type  the message type as string.
    content   the message content as dictionary.
    data      raw bytes.

    """
    if msg_type == 'request_file_spectrogram':
      self.on_file_spectrogram(**content)
    elif msg_type == 'request_data_spectrogram':
      self.on_data_spectrogram(data, **content)
    else:
      super(self.__class__, self).receive_message(
          msg_type, content, data)

  def send_spectrogram(self, spec, fs, length, canvas_id=None):
    spec = downsample(spec)
    spec = astype(spec)
    self.send_message('spectrogram',
                      {'extent': spec.shape,
                       'fs': fs,
                       'length': length,
                       'canvas_id': canvas_id},
                      spec.tostring())

  def send_progress(self, progress, canvas_id=None):
    self.send_message('loading_progress', {
        'progress': progress,
        'canvas_id': canvas_id})

  def on_file_spectrogram(self, filename, nfft=1024,
                          overlap=0.5, dataType=AUDIO):
    """Loads an audio file and calculates a spectrogram.

    Arguments:
    filename  the file name from which to load the audio data.
    nfft      the FFT length used for calculating the spectrogram.
    overlap   the amount of overlap between consecutive spectra.

    """

    dataHandlers = {
        AUDIO: self.on_audio_file_spectrogram,
        EEG: self.on_eeg_file_spectrogram,
    }

    handler = dataHandlers.get(dataType)
    if handler is None:
      return

    try:
      handler(filename, nfft, overlap)
    except RuntimeError as e:  # error loading file
      error_msg = 'Filename: {} could not be loaded.\n{}'.format(filename, e)
      self.send_message('error', {
          'error_msg': error_msg
      })
      print(error_msg)

  def _load_spectrofile(self, filename):
    f = h5py.File(filename, 'r')
    data = f['data']
    fs = f['Fs'][0][0]
    print 'Loaded spectral file %s' % filename
    return data, fs

  def on_eeg_file_spectrogram(self, filename, nfft, overlap):
    data, fs = self._load_spectrofile(filename)
    data = data[:NUM_SAMPLES]  # ok lets just chunk a bit of this mess

    nfft, shift = get_spectrogram_params(fs)

    for ch in CHANNELS:
      T = []
      pairs = DIFFERENCE_PAIRS.get(ch)
      for i, pair in enumerate(pairs):
        c1, c2 = pair
        # take differences between the channels (columns) in each of the
        # regions
        diff = data[:, CHANNEL_INDEX.get(c2)] - data[:, CHANNEL_INDEX.get(c1)]
        T.append(self.spectrogram(diff, nfft, shift, canvas_id=ch, log=False))
        progress = (i + 1) / len(pairs)
        self.send_progress(progress, canvas_id=ch)
      # compute the regional average of the spectrograms for each channel
      self.send_spectrogram(sum(T) / 4, fs, NUM_SAMPLES / fs, canvas_id=ch)

  def on_audio_file_spectrogram(self, filename, nfft, overlap):
    _file = SoundFile(filename)
    self._audio_file_spectrogram(_file, nfft, overlap)

  def _audio_file_spectrogram(self, _file, nfft, overlap):
    sound = _file[:].sum(axis=1)
    shift = round(nfft * overlap)
    spec = self.spectrogram(sound, nfft, shift)
    fs = _file.sample_rate
    self.send_spectrogram(spec, fs, len(_file) / fs)

  def on_data_spectrogram(self, data, nfft=1024, overlap=0.5, dataType=AUDIO):
    """Loads an audio file and calculates a spectrogram.

    Arguments:
    data      the content of a file from which to load data.
    nfft      the FFT length used for calculating the spectrogram.
    overlap   the amount of overlap between consecutive spectra.

    """
    dataHandlers = {
        AUDIO: self.on_audio_data_spectrogram,
        EEG: self.on_eeg_data_spectrogram,
    }
    handler = dataHandlers.get(dataType)
    if handler is None:
      return
    handler(data, nfft, overlap)

  def on_eeg_data_spectrogram(self, data, nfft, overlap):
    """
      Convert the raw data from a file into an array of four spectrograms.
      The data file is a matrix of voltage data with a column per sensor.
      We computer the differences between sensors in the four
      regions (LL, LP, RL, RP)
      and then average the spectrograms for each of these regions.
      We send each spectrogram to the client for visualization.
    """
    # TODO (joshblum): figure out something not as stupid... (can work
    # concurrently?)
    # this doesn't work at all browser limit for files is pretty small...
    FILENAME = 'tmp-data'
    with open(FILENAME, 'w') as f:
      f.write(data)

    self.on_eeg_file_spectrogram(FILENAME, nfft, overlap)

  def on_audio_data_spectrogram(self, data, nfft, overlap):
    _file = SoundFile(io.BytesIO(data), virtual_io=True)
    self._audio_file_spectrogram(_file, nfft, overlap)

  def spectrogram(self, data, nfft, shift, canvas_id=None, log=True):
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
    num_blocks = int((len(data) - nfft) / shift + 1)
    window = hann(nfft)
    specs = np.zeros((nfft / 2 + 1, num_blocks), dtype=np.float32)
    for idx in xrange(num_blocks):
      specs[:, idx] = np.abs(np.fft.rfft(
          data[idx * shift:idx * shift + nfft] * window, n=nfft)) / nfft
      if log and idx % 10 == 0:
        self.send_progress(idx / num_blocks, canvas_id)
    specs[:, -1] = np.abs(
        np.fft.rfft(data[num_blocks * shift:], n=nfft)) / nfft
    if log:
      self.send_progress(1, canvas_id)
    return specs.T

if __name__ == '__main__':
  import os
  import webbrowser
  from tornado.web import Application
  from tornado.ioloop import IOLoop
  import random

  app = Application([('/spectrogram', SpectrogramWebSocket)])

  random.seed()
  port = random.randrange(49152, 65535)
  app.listen(port)
  webbrowser.open('file://{}/main.html?port={}'.format(os.getcwd(), port))
  IOLoop.instance().start()
