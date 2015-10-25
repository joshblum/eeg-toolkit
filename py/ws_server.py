from __future__ import division

import time
import json

import eeg_compute as eeg_compute

from constants import CHANNELS

from helpers import astype
from helpers import downsample
from helpers import downsample_extent
from helpers import from_bytes
from helpers import to_bytes

from tornado.websocket import WebSocketHandler
from tornado.ioloop import IOLoop
from tornado.web import Application
from tornado.web import StaticFileHandler
from tornado.web import RequestHandler


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
  - request_file_spectrogram, which needs a data identifier, and optionally
    `nfft` and `overlap`.
  - request_data_spectrogram, which needs the file as a binary data
    blob, and optionally `nfft` and `overlap`.

  """

  def receive_message(self, msg_type, content, data=None):
    """Message dispatcher.

    Dispatches
    - `request_file_spectrogram` to self.on_eeg_spectrogram

    Arguments:
    msg_type  the message type as string.
    content   the message content as dictionary.
    data      raw bytes.

    """
    if msg_type == 'request_file_spectrogram':
      self.on_eeg_spectrogram(**content)
    else:
      super(self.__class__, self).receive_message(
          msg_type, content, data)

  def send_spectrogram_new(self, spec_params, canvas_id=None):
    nblocks, nfreqs = downsample_extent(
        spec_params.nblocks, spec_params.nfreqs)
    print 'spec_new:::shape:', (nblocks, nfreqs), 'ch:', canvas_id
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
    print 'spec_update:::shape:', spec.shape, 'ch:', canvas_id
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

  def on_eeg_spectrogram(self, mrn, nfft=1024,
                         duration=None, overlap=0.5, dataType=EEG):
    """

    Arguments:
    mrn       the medical record number from which to load the data.
    nfft      the FFT length used for calculating the spectrogram.
    duration  the duration (in hours) to compute. If `None` the entire
    overlap   the amount of overlap between consecutive spectra.

    """
    t0 = time.time()
    dataHandlers = {
        EEG: self.on_eeg_file_spectrogram,
    }

    handler = dataHandlers.get(dataType)
    if handler is None:
      return

    try:
      handler(mrn, nfft, duration, overlap)
      t1 = time.time()
      print 'Total time:', t1 - t0
    except RuntimeError as e:  # error loading file
      error_msg = 'Data: {} could not be loaded.\n{}'.format(mrn, e)
      self.send_message('error', {
          'error_msg': error_msg
      })
      print(error_msg)

  def on_eeg_file_spectrogram(self, mrn, nfft, duration, overlap):
    spec_params = eeg_compute.get_eeg_spectrogram_params(mrn, duration)

    for ch, ch_id in CHANNELS.iteritems():
      self.send_spectrogram_new(spec_params, canvas_id=ch)
      # TODO(joshblum): chunk data?
      self.send_progress(0.1, ch)
      spec = eeg_compute.eeg_spectrogram_as_arr(spec_params, ch_id)
      self.send_spectrogram_update(spec, canvas_id=ch)
      self.send_progress(1, ch)

    eeg_compute.close_edf(mrn)


class MainHandler(RequestHandler):

  def get(self):
    self.render('html/main.html')


def make_app():
  handlers = [
      (r'/', MainHandler),
      (r'/compute/spectrogram/', SpectrogramWebSocket),
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
