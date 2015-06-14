import numpy
import scipy.io

from collections import namedtuple

CpData = namedtuple(
    'CpData', ['r', 'ch', 't01', 'cp', 'yp', 'cu', 'cl', 'mu', 'm', 'tt'])


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
  tt = f['tt'][0]
  return CpData(r, ch, t01, cp, yp, cu, cl, mu, m, tt)


def get_change_points(r, ch, t01):
  minimum = 5000
  stride = 10
  s = numpy.sum(r[ch], axis=0)
  s = numpy.minimum(s, minimum)
  s = s[::stride]
  tt = t01[::stride]

  Nt = len(tt)

  b = 0.95
  bb = 0.995
  max_amp = 3000

  # CUMSUM parameters
  sigma = 100
  K = 4 * sigma / 2
  h = 5
  H = h * sigma

  m = numpy.zeros(Nt)
  m[0] = 1000
  mu = numpy.zeros(Nt)
  mu[0] = 1000

  cu = numpy.zeros(Nt)
  cl = numpy.zeros(Nt)
  cp = []
  yp = []
  np = 0.
  nm = 0.
  ct = 0
  for j in xrange(1, Nt):
    ct += 1

    # signal we are tracking -- short term mean
    m[j] = min(m[j - 1] * b + (1 - b) * s[j], max_amp)
    if ct < 20:
      mu[j] = min(mu[j - 1] * bb + (1 - bb) * m[j], max_amp)
    else:
      mu[j] = mu[j - 1]

    cu[j] = max(0, cu[j - 1] + m[j] - mu[j] - K)
    cl[j] = max(0, cl[j - 1] + mu[j] - m[j] - K)

    np = (np + 1) * (cu[j] > 0)
    nm = (nm + 1) * (cl[j] > 0)
    if cu[j] > H or cl[j] > H:
      cp.append(tt[j])
      yp.append(max_amp)
      ct = 0
      if cu[j] > H:
        mu[j] = mu[j] + K + cu[j] / np
        cu[j] = 0.
        np = 0.
      if cl[j] > H:
        mu[j] = mu[j] - K - cl[j] / nm
        cl[j] = 0.
        nm = 0.
  return CpData(r, ch, t01,
                numpy.array(cp), numpy.array(yp),
                cu, cl, mu, m, tt)


def verify_result(cp_data, res):
  print 'Verifying results.'
  cp_data = cp_data._asdict()
  res = res._asdict()
  for k1, v1 in cp_data.iteritems():
    v2 = res[k1]
    if isinstance(v1, numpy.ndarray):
      valid = numpy.array_equal(v1, v2)
      if not valid:
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


def main():
  filename = 'cp-io.mat'
  cp_data = load_file(filename)
  res = get_change_points(cp_data.r, cp_data.ch, cp_data.t01)
  verify_result(cp_data, res)

if __name__ == '__main__':
  main()
