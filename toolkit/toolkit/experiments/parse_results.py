from collections import defaultdict
import pprint

DELIMITER = 'experiment_data::'


def parse_log(filename):
  '''
      Open the given file and return lines that match DELIMITER, stripping
      this from the beginning of the line.
  '''
  lines = []

  try:
    with open(filename) as f:
      for line in f:
        if line.startswith(DELIMITER):
          line = line[len(DELIMITER):]
          lines.append(line.rstrip())
  except IOError:
    pass

  return lines


def bytes_to_gb(byte_str):
  return '%sgb' % (int(byte_str) / 10**9)


def bytes_to_mb(byte_str):
  """
    The logs  have an error where the value is / sizeof(float).
    We multiply here to compensate
  """
  return '%smb' % (4 * int(byte_str) / 10**6)


def get_desired_size_from_mrn(mrn):
  '''
    For a mrn of the form xxx-xgb.xxx, returns the strinng 'xgb'
  '''
  mrn = mrn.split('.')[0]
  return mrn.split('-')[-1]


def ingestion_parser(lines):
  '''
      Lines are given in a CSV format of:
      mrn,backend_name,desired_size(gb),read_chunk_size(mb),time Returns a
      dictionary of (backend_name, desired_size, read_chunk_size) -> time.
  '''
  data = defaultdict(list)
  for line in lines:
    mrn, backend_name, desired_size, read_chunk_size, time = line.split(',')
    key = (backend_name, bytes_to_gb(desired_size),
           bytes_to_mb(read_chunk_size))
    value = float(time)
    data[key].append(value)
  return data


def precompute_parser(lines):
  '''
      Lines are given in a CSV format of:
      mrn,backend_name,write_chunk_size(mb),time,tag Returns a dictionary of
      (tag, backend_name, desired_size, write_chunk_size) -> time.
  '''
  data = defaultdict(list)
  for line in lines:
    mrn, backend_name, write_chunk_size, time, tag = line.split(
        ',')
    desired_size = get_desired_size_from_mrn(mrn)
    key = (tag, backend_name, desired_size, bytes_to_mb(write_chunk_size))
    value = float(time)
    data[key].append(value)

    if 'read_time' in tag:
      new_key = list(key)
      new_key[0] = 'read_time'
      new_key = tuple(new_key)
      if not len(data[new_key]):
        data[new_key] = [0]
      data[new_key][0] += value

  return data


def avg_values(values):
  '''
  Takes a dictionary of key-value pair (...) -> [...]
  and returns the average of the values in the list
  '''
  avg_vals = {}
  for k, v in values.iteritems():
    avg_vals[k] = sum(v) / len(v)
  return avg_vals

EXPERIMENTS = {
    'ingestion': ('ingestion_results.txt', ingestion_parser),
    'precompute': ('precompute_results.txt', precompute_parser),
}


def main(backend_name):
  results = {}
  for exp_name, value_pair in EXPERIMENTS.iteritems():
    filename, parser = value_pair
    filename += '-%s-' % backend_name
    i = 1
    values = {}
    while True:
      lines = parse_log(filename + str(i))
      if not len(lines):
        break
      values = parser(lines)
      i += 1
    results[exp_name] = avg_values(values)
  return results


if __name__ == '__main__':
  import sys
  if len(sys.argv) != 2:
    print 'usage: python parse_results.py [BinaryBackend|HDF5Backend|TileDBBackend]'
  else:
    backend_name = sys.argv[1]
    results = main(backend_name)
    pprint.pprint(results)
