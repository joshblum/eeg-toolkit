import io
import json
import h5py
import struct
import numpy as np

from tornado.websocket import WebSocketHandler
from pysoundfile import SoundFile

NUM_SAMPLES = 200 * 60 * 60  # 200Hz * 60s * 60m = 1hr of data

AUDIO = "audio"
EEG = "eeg"

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


def hann(n):
  return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(n) / (n - 1))


def from_bytes(b):
  return struct.unpack("@i", b)[0]


def to_bytes(n):
  return struct.pack("@i", n)


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
    print("WebSocket opened")

  def send_message(self, msg_type, content, data=None):
    """Send a message.

    Arguments:
    msg_type  the message type as string.
    content   the message content as json-serializable data.
    data      raw bytes that are appended to the message.

    """

    if data is None:
      self.write_message(json.dumps({"type": msg_type,
                                     "content": content}).encode())
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

    if msg_type == "information":
      print(content)
    else:
      print(("Don't know what to do with message of type {}" +
             "and content {}").format(msg_type, content))

  def on_close(self):
    print("WebSocket closed")


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
    print "dataType:", dataType

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
    Fs = f['Fs'][0][0]
    print "Loaded spectral file %s" % filename
    return data, Fs

  def on_eeg_file_spectrogram(self, filename, nfft, overlap):
    data, fs = self._load_spectrofile(filename)
    data = data[:NUM_SAMPLES]  # ok lets just chunk a bit of this mess

    print "Calculating bioploar data..."
    # take differences between the channels (columns) in each of the regions
    bipolar_data = {ch: [
        data[:, CHANNEL_INDEX.get(c2)] - data[:, CHANNEL_INDEX.get(c1)]
        for c1, c2 in pairs]
        for ch, pairs in DIFFERENCE_PAIRS.iteritems()
    }

    print "Calculated bioploar data"
    # now store the spectrogram for each of the channels
    spec_data = dict.fromkeys(CHANNELS)

    for ch, diffs in bipolar_data.iteritems():
      l = []
      print "Computing spectrogram for ch: %s" % ch
      for diff in diffs:
        spectrogram = self.spectrogram(diff, nfft, overlap)
        print "spectrogram shape:", spectrogram.shape
        l.append(spectrogram)
      spec_data[ch] = l

    # from each of the spectrograms, average the channels within each region
    reg_avg = dict.fromkeys(CHANNELS)

    for ch, spec_data in spec_data.iteritems():
      T = np.zeros(spec_data[0].shape)
      print "Computing regional average for ch: %s" % ch
      for s in spec_data:
        T += s
      reg_avg[ch] = T / 4

    # TODO (joshblum): display all 4 images for the channels
    spec = reg_avg[ch]
    self.send_message('spectrogram',
                      {'extent': spec.shape,
                       'fs': fs,
                       'length': len(data) / fs},
                      spec.tostring())

  def on_audio_file_spectrogram(self, filename, nfft, overlap):
    file = SoundFile(filename)
    sound = file[:].sum(axis=1)
    spec = self.spectrogram(sound, nfft, overlap)

    self.send_message('spectrogram',
                      {'extent': spec.shape,
                       'fs': file.sample_rate,
                       'length': len(file) / file.sample_rate},
                      spec.tostring())

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
      We return a 4 x 1 array of these spectrograms for visualization
    """
    # TODO (joshblum): figure out something not as stupid... (can work
    # concurrently?)
    # this doesn't work at all browser limit for files is pretty small...
    FILENAME = 'tmp-data'
    with open(FILENAME, 'w') as f:
      f.write(data)

    self.on_file_eeg_spectrogram(FILENAME, nfft, overlap)

  def on_audio_data_spectrogram(self, data, nfft, overlap):
    file = SoundFile(io.BytesIO(data), virtual_io=True)
    sound = file[:].sum(axis=1)
    spec = self.spectrogram(sound, nfft, overlap)

    self.send_message('spectrogram',
                      {'extent': spec.shape,
                       'fs': file.sample_rate,
                       'length': len(file) / file.sample_rate},
                      spec.tostring())

  def spectrogram(self, data, nfft, overlap):
    """Calculate a real spectrogram from audio data

    An audio data will be cut up into overlapping blocks of length
    `nfft`. The amount of overlap will be `overlap*nfft`. Then,
    calculate a real fourier transform of length `nfft` of every
    block and save the absolute spectrum.

    Arguments:
    data      audio data as a numpy array.
    nfft      the FFT length used for calculating the spectrogram.
    overlap   the amount of overlap between consecutive spectra.

    """

    shift = round(nfft * overlap)
    num_blocks = int((len(data) - nfft) / shift + 1)
    specs = np.zeros((nfft / 2 + 1, num_blocks), dtype=np.float32)
    window = hann(nfft)
    for idx in range(num_blocks):
      specs[:, idx] = np.abs(
          np.fft.rfft(
              data[idx * shift:idx * shift + nfft] * window,
              n=nfft)) / nfft
      if idx % 10 == 0:
        self.send_message(
            "loading_progress", {"progress": idx / num_blocks})
    specs[:, -1] = np.abs(
        np.fft.rfft(data[num_blocks * shift:], n=nfft)) / nfft
    self.send_message("loading_progress", {"progress": 1})
    return specs.T

if __name__ == "__main__":
  import os
  import webbrowser
  from tornado.web import Application
  from tornado.ioloop import IOLoop
  import random

  app = Application([("/spectrogram", SpectrogramWebSocket)])

  random.seed()
  port = random.randrange(49152, 65535)
  app.listen(port)
  webbrowser.open('file://{}/main.html?port={}'.format(os.getcwd(), port))
  IOLoop.instance().start()
