from collections import defaultdict
import pprint

# Pretty print class names
CLASS_NAME_MAP = {
    'AbstractStorageBackend<ArrayMetadata>*': 'BinaryBackend'
}
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
          lines.append(line)
  except IOError:
    print 'File %s not found.' % filename

  return lines


def bytes_to_gb(byte_str):
  return '%sgb' % (int(byte_str) / 10**9)


def bytes_to_mb(byte_str):
  return '%smb' % (int(byte_str) / 10**6)


def ingestion_parser(lines):
  '''
      Lines are given in a CSV format of: mrn,backend_name,desired_size(gb),read_chunk_size(mb),time
      Returns a dictionary of (backend_name, desired_size, read_chunk_size) -> time.
  '''
  values = defaultdict(list)
  for line in lines:
    mrn, backend_name, desired_size, read_chunk_size, time = line.split(',')
    key = (CLASS_NAME_MAP.get(backend_name), bytes_to_gb(
        desired_size), bytes_to_mb(read_chunk_size))
    value = float(time)
    values[key].append(value)
  return values


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
}


def main():
  for filename, parser in EXPERIMENTS.itervalues():
    lines = parse_log(filename)
    values = parser(lines)
    pprint.pprint(avg_values(values))


if __name__ == '__main__':
  main()
