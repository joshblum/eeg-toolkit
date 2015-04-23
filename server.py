from __future__ import division

import io
import json
import time

import numpy as np
from pysoundfile import SoundFile

from tornado.ioloop import IOLoop
from tornado.websocket import WebSocketHandler
from tornado.web import Application
from tornado.web import StaticFileHandler
from tornado.web import RequestHandler

import compute.eeg_compute as eeg_compute

from helpers import astype
from helpers import from_bytes
from helpers import to_bytes
from helpers import grouper
from helpers import downsample
from helpers import downsample_extent

from spectrogram import eeg_ch_spectrogram
from spectrogram import get_audio_spectrogram_params
from spectrogram import get_eeg_spectrogram_params
from spectrogram import spectrogram
from spectrogram import load_h5py_spectrofile
from spectrogram import CHANNELS

AUDIO = 'audio'
EEG = 'eeg'


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

  def send_spectrogram_new(self, spec_params, canvas_id=None):
    nblocks, nfreqs = downsample_extent(
        spec_params.nblocks, spec_params.nfreqs)
    print "spec_new:::shape:", (nblocks, nfreqs), "ch:", canvas_id
    self.send_message('spectrogram',
                      {'action': 'new',
                       'nblocks': nblocks,
                       'nfreqs': nfreqs,
                       'fs': spec_params.fs,
                       'length': spec_params.spec_len,
                       'canvasId': canvas_id},
                      None)

  def send_spectrogram_update(self, spec, canvas_id=None):
    spec = downsample(spec)
    spec = astype(spec)
    nblocks, nfreqs = spec.shape
    print "spec_update:::shape:", spec.shape, "ch:", canvas_id
    self.send_message('spectrogram',
                      {'action': 'update',
                       'nblocks': nblocks,
                       'nfreqs': nfreqs,
                       'canvasId': canvas_id},
                      spec.tostring())

  def send_progress(self, progress, canvas_id=None):
    self.send_message('loading_progress', {
        'progress': progress,
        'canvasId': canvas_id})

  def on_file_spectrogram(self, filename, nfft=1024,
                          duration=None, overlap=0.5, dataType=AUDIO):
    """Loads an audio file and calculates a spectrogram.

    Arguments:
    filename  the file name from which to load the audio data.
    nfft      the FFT length used for calculating the spectrogram.
    duration  the duration (in hours) to compute. If `None` the entire
    overlap   the amount of overlap between consecutive spectra.

    """
    t0 = time.time()
    dataHandlers = {
        AUDIO: self.on_audio_file_spectrogram,
        EEG: self.on_eeg_file_spectrogram,
    }

    handler = dataHandlers.get(dataType)
    if handler is None:
      return

    try:
      handler(filename, nfft, duration, overlap)
      t1 = time.time()
      print "Total time:", t1 - t0
    except RuntimeError as e:  # error loading file
      error_msg = 'Filename: {} could not be loaded.\n{}'.format(filename, e)
      self.send_message('error', {
          'error_msg': error_msg
      })
      print(error_msg)

  def on_eeg_file_spectrogram(self, filename, nfft, duration, overlap):
    spec_params = eeg_compute.get_eeg_spectrogram_params(filename, duration)

    for ch, ch_id in CHANNELS.iteritems():
      self.send_spectrogram_new(spec_params, canvas_id=ch)
      # TODO(joshblum): chunk data?
      self.send_progress(0.1, ch)
      print "CH:", ch
      spec = eeg_compute.eeg_spectrogram_handler(spec_params, ch_id)
      self.send_spectrogram_update(spec, canvas_id=ch)
      self.send_progress(1, ch)

    eeg_compute.close_edf(filename)

  def on_audio_file_spectrogram(self, filename, nfft, duration, overlap):
    _file = SoundFile(filename)
    self._audio_file_spectrogram(_file, nfft, duration, overlap)

  def _audio_file_spectrogram(self, _file, nfft, duration, overlap):
    # first get the spec_params and let the client setup the canvas
    fs = _file.sample_rate
    spec_params = get_audio_spectrogram_params(
        _file, fs, duration, nfft, overlap)
    self.send_spectrogram_new(spec_params)

    # now lets compute the spectrogram and send it over
    data = _file[:spec_params.nsamples].sum(axis=1)
    for chunk in grouper(data, spec_params.chunksize, spec_params.nfft):
      chunk = np.array(chunk)
      spec = spectrogram(chunk, spec_params)
      self.send_spectrogram_update(spec)

  def on_data_spectrogram(self, data, nfft=1024, duration=None,
                          overlap=0.5, dataType=AUDIO):
    """Loads an audio file and calculates a spectrogram.

    Arguments:
    data      the content of a file from which to load data.
    nfft      the FFT length used for calculating the spectrogram.
    duration  the duration (in hours) to compute. If `None` the entire
    data source is used.
    overlap   the amount of overlap between consecutive spectra.

    """
    dataHandlers = {
        AUDIO: self.on_audio_data_spectrogram,
        EEG: self.on_eeg_data_spectrogram,
    }
    handler = dataHandlers.get(dataType)
    if handler is None:
      return
    handler(data, nfft, duration, overlap)

  def on_eeg_data_spectrogram(self, data, nfft, duration, overlap):
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

    self.on_eeg_file_spectrogram(FILENAME, nfft, duration, overlap)

  def on_audio_data_spectrogram(self, data, nfft, duration, overlap):
    _file = SoundFile(io.BytesIO(data), virtual_io=True)
    self._audio_file_spectrogram(_file, nfft, overlap)


class MainHandler(RequestHandler):

  def get(self):
    self.render('html/main.html')


def make_app():
  handlers = [
      (r'/', MainHandler),
      (r'/spectrogram', SpectrogramWebSocket),
      (r'/(.*)', StaticFileHandler, {
          'path': ''
      }),
  ]
  return Application(handlers, autoreload=True)


def main():
  app = make_app()
  port = 5000
  app.listen(port)
  print 'Listing on port:', port
  IOLoop.current().start()

if __name__ == '__main__':
  main()
