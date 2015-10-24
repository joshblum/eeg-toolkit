import numpy
import scipy.io

from collections import namedtuple

import compute.eeg_compute as compute

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


def get_change_points(spec_mat):
  s = numpy.sum(spec_mat, axis=0)

  minimum = 5000
  stride = 10
  nt = int(round(len(s) / stride, -1))  # round to the tens place

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


def verify_result(res1, res2, src=''):
  print 'Verifying results.'
  print '-' * 45
  res1 = res1._asdict()
  res2 = res2._asdict()
  for k1, v1 in res1.iteritems():
    v2 = res2.get(k1)
    if v2 is None:
      continue
    if isinstance(v1, numpy.ndarray):
      valid = numpy.array_equal(v1, v2)
      if not valid and len(v1) == len(v2):
        valid = numpy.allclose(v1, v2)
        if not valid:
          print 'invalid', v1[:5]
          print 'invalid', v2[:5]
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

  print 'Results %s verified.' % src


def main():
  ml_filename = 'cp-io.mat'
  ml_res = load_file(ml_filename)
  spec_mat = ml_res.r[ml_res.ch]
  # py_res = get_change_points(spec_mat)
  compute.example_change_points_as_arr(spec_mat)
  # verify_result(ml_res, py_res, 'py_res')

if __name__ == '__main__':
  main()
