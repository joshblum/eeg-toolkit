from watchdog.observers import Observer
from watchdog.events import PatternMatchingEventHandler

import subprocess
import os
import time

BASE_DIR = '/home/ubuntu/eeg-data/eeg-data/'


class FileConverter(PatternMatchingEventHandler):

  def on_moved(self, event):
    self.convert(event.dest_path)

  def on_created(self, event):
    self.convert(event.src_path)

  def convert(self, path):
    if not path.endswith('.edf'):
      return
    filename = os.path.split(path)[-1]
    mrn = filename.split('.edf')[0]

    if not os.path.isfile('./edf_converter'):
      subprocess.call(['make', 'edf_converter'])
    subprocess.call(['./edf_converter', mrn])

    if not os.path.isfile('./precompute_spectrogram'):
      subprocess.call(['make', 'precompute_spectrogram'])
    subprocess.call(['./precompute_spectrogram', mrn])


def main():
  """
    Watch the directory `BASE_DIR` for files with the extension `*.edf` and
    convert them to the appropiate `StorageBackend` format and precompute the
    spectrogram results.
  """
  event_handler = FileConverter('*.edf')

  observer = Observer()
  observer.schedule(event_handler, BASE_DIR)
  observer.start()

  try:
    while True:
      time.sleep(1)
  except KeyboardInterrupt:
    observer.stop()
  observer.join()

if __name__ == '__main__':
  main()
