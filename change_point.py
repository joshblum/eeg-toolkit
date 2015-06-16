import numpy
import scipy.io

from collections import namedtuple

from compute.eeg_compute import eeg_spectrogram_handler_as_arr
from compute.eeg_compute import get_eeg_spectrogram_params

CpData = namedtuple(
    'CpData', ['r', 'ch', 't01',
               'cp', 'yp', 'cu',
               'cl', 'mu', 'm',
               'total_count'])


def load_file(filename):
  f = scipy.io.loadmat(filename)
  r = f['r'][0]
  ch = f['ch'][0][0] - 1  # matlab vs python indexing
  t01 = f['t01'][0]

  cp = f['cp'][0]
  yp = f['yp'][0]
  cu = f['cu'][0]
  cl = f['cl'][0]
  mu = f['mu'][0]
  m = f['m'][0]
  total_count = len(yp)
  return CpData(r, ch, t01, cp, yp, cu, cl, mu, m, total_count)


def get_change_points_matlab(r, ch, t01):
  tt = t01[::10]  # stride = 10
  nt = len(tt)
  return get_change_points(r[ch])


def get_change_points(spec_mat):
  s = numpy.sum(spec_mat, axis=0)

  minimum = 5000
  stride = 10
  nt = (len(s) / stride) + 1

  b = 0.95
  bb = 0.995
  max_amp = 3000

  # CUMSUM parameters
  sigma = 100
  K = 4 * sigma / 2
  h = 5
  H = h * sigma

  m = numpy.zeros(nt)
  m[0] = 1000
  mu = numpy.zeros(nt)
  mu[0] = 1000

  cu = numpy.zeros(nt)
  cl = numpy.zeros(nt)

  cp = []

  ct = 0
  total_count = 0

  np = 0.
  nm = 0.

  for j in xrange(1, nt):
    ct += 1
    s_j = s[j * stride]
    if s_j > minimum:
      s_j = minimum

    # signal we are tracking -- short term mean
    m[j] = min(m[j - 1] * b + (1 - b) * s_j, max_amp)
    if ct < 20:
      mu[j] = min(mu[j - 1] * bb + (1 - bb) * m[j], max_amp)
    else:
      mu[j] = mu[j - 1]

    cu[j] = max(0, cu[j - 1] + m[j] - mu[j] - K)
    cl[j] = max(0, cl[j - 1] + mu[j] - m[j] - K)

    np = (np + 1) * (cu[j] > 0)
    nm = (nm + 1) * (cl[j] > 0)

    if cu[j] > H or cl[j] > H:
      total_count += 1
      cp.append(j * stride)
      ct = 0

      if cu[j] > H:
        mu[j] = mu[j] + K + cu[j] / np
        cu[j] = 0.
        np = 0.
      if cl[j] > H:
        mu[j] = mu[j] - K - cl[j] / nm
        cl[j] = 0.
        nm = 0.

  yp = [max_amp] * len(cp)

  return CpData(None, None, None,
                numpy.array(cp), numpy.array(yp),
                cu, cl, mu, m, total_count)


def verify_result(cp_data, res):
  print 'Verifying results.'
  print '-' * 45
  cp_data = cp_data._asdict()
  res = res._asdict()
  for k1, v1 in cp_data.iteritems():
    v2 = res[k1]
    if v2 is None:
        continue
    if isinstance(v1, numpy.ndarray):
      valid = numpy.array_equal(v1, v2)
      if not valid and len(v1) == len(v2):
        valid = numpy.allclose(v1, v2)
      r1 = v1.shape
      r2 = v2.shape
    else:
      valid = v1 == v2
      r1 = v1
      r2 = v2

    if not valid:
      print 'incorrect result, key:', k1
      print 'original: %s, computed: %s' % (r1, r2)
      print '-' * 45

  print 'Results verified.'


def test_cpp():
  # filename = '/home/ubuntu/MIT-EDFs/MIT-CSAIL-007.edf'
  filename = '/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-007.edf'
  duration = 4.0
  spec_params = get_eeg_spectrogram_params(filename, duration)
  spec_mat = eeg_spectrogram_handler_as_arr(spec_params, 0)  # channel LL
  cpp_res = get_change_points(spec_mat)
  print 'Total count:', cpp_res.total_count


def main():
  test_cpp()

  matlab_filename = 'cp-io.mat'
  cp_data = load_file(matlab_filename)
  matlab_res = get_change_points_matlab(cp_data.r, cp_data.ch, cp_data.t01)

  verify_result(cp_data, matlab_res)

if __name__ == '__main__':
  main()
