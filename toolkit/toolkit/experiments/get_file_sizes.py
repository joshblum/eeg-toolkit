import glob
import os
import json
import pprint
from collections import defaultdict

DATADIR = '/home/ubuntu/eeg-data/eeg-data'
FILE_PATTERNS = {
    'TileDBBackend': '-tiledb',
    'HDF5Backend': '.h5',
    'BinaryBackend': '.bin',
}

DUMP_FILE = '%s-file-sizes.json'


def get_file_size(path):
  # http://stackoverflow.com/questions/1392413/calculating-a-directory-size-using-python
  if os.path.isdir(path):
      total_size = 0
      for dirpath, dirnames, filenames in os.walk(path):
        for f in filenames:
          fp = os.path.join(dirpath, f)
          total_size += os.path.getsize(fp)
  else:
      total_size = os.path.getsize(path)
  return total_size


def sizeof_fmt(num, suffix='B'):
  # http://stackoverflow.com/questions/1094841/reusable-library-to-get-human-readable-version-of-file-size
  for unit in ['', 'K', 'M', 'G', 'T', 'P', 'E', 'Z']:
    if abs(num) < 1024.0:
      return "%3.1f %s%s" % (num, unit, suffix)
    num /= 1024.0
  return "%.1f %s%s" % (num, 'Y', suffix)


def get_file_sizes(backend_name):
  file_ext = FILE_PATTERNS[backend_name]
  filenames = glob.glob('%s/*%s' % (DATADIR, file_ext))
  results = {}
  for filename in filenames:
    if 'cached' in filename:
      continue
    key = os.path.basename(filename).replace(file_ext, '').split('-')
    if len(key) < 2:
      continue
    else:
      key = key[1]  # 005-xgb-backend_name
    file_size = get_file_size(filename)
    results[key] = {
        'file_size': file_size,
        'human_size': sizeof_fmt(file_size)
    }
  return results


def store_file_sizes(backend_name, file_sizes):
  with open(DUMP_FILE % backend_name, 'w') as f:
    f.write(json.dumps(file_sizes))


def extract_file_sizes(backend_name):
  try:
    with open(DUMP_FILE % backend_name) as f:
      return json.loads(f.read())
  except IOError:
    return {}


if __name__ == '__main__':
  import sys
  if len(sys.argv) == 2:
    backend_name = sys.argv[1]
    file_sizes = get_file_sizes(backend_name)
    store_file_sizes(backend_name, file_sizes)
    pprint.pprint(file_sizes)
  elif len(sys.argv) == 1:
    file_sizes = {}
    for backend_name in FILE_PATTERNS:
      file_sizes[backend_name] = extract_file_sizes(backend_name)
    pprint.pprint(file_sizes)
  else:
    print 'usage: python get_file_sizes.py [BinaryBackend|HDF5Backend|TileDBBackend]'
