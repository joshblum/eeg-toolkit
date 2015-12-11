
# coding: utf-8

# In[132]:

from collections import defaultdict
import csv
import math

import numpy as np
from sklearn import linear_model
from sklearn.externals import joblib


inverse_fn = np.vectorize(lambda x: 1/x)

def csv_to_dict(filename):
    """
        dict structure is {
            '11' : { # extent value
                'memory' : [...],
                ...
            },
            ...
        }
    """
    data = defaultdict(lambda : defaultdict(list))
    with open(filename) as f:
        reader = csv.DictReader(f)
        for line in reader:
            line_data = data[int(line['extent'])]
            for k, v in line.iteritems():
                line_data[k].append(float(v))
    return data

def fit(x, y, extent, change_basis=False, test_set_size=None):
    if test_set_size is None:
        x_train, x_test = x, x
        y_train, y_test = y, y
    else:
        x_train, x_test = x[:-test_set_size], x[-test_set_size:]
        y_train, y_test = y[:-test_set_size], y[-test_set_size:]

    if change_basis:
        x_test_transform = inverse_fn(x_test)
        x_train = inverse_fn(x_train)

    regr = linear_model.TheilSenRegressor()
    regr.fit(x_train, y_train)
#     print regr.score(x_test_transform, y_test)
    
    return regr

def fit_xy(xy):
    """
    Given a numpy array of form [ x, y ] representing all samples for all
    extents, return a dictionary keyed by extent where the values are the
    regression models trained on samples in x and y.
    """
    x, y = xy[0, :, :], xy[1, :, :]
    nmodels = x.shape[0]
    nsamples = x.shape[1]
    regrs = {}
    for i in range(nmodels):
        extent = i + 1        
        x_col = x[i, :].reshape((nsamples, 1))
        y_col = y[i, :]
        regrs[extent] = fit(x_col, y_col, extent, change_basis=True, test_set_size=25)
    
    return regrs

def save_and_load_xy(filename, x_key, y_key, max_extent):
    """
    Load a 3-D numpy array from either numpy or csv. If the numpy serialization
    doesn't exist yet, save it.

    The numpy array has form [ x, y ], where x and y have dimensions: number of
    extents x number of samples.
    """
    import os
    npy_filename = "vg-{0}-{x}-{y}.npy".format(filename, x=x_key, y=y_key)
    # If we already serialized the numpy array.
    if os.path.exists(npy_filename):
        with open(npy_filename, 'r') as f:
            xy = np.load(f)
        return xy
    
    # Otherwise, we're reading from a CSV. Then, also serialize and save it as a numpy array.
    csv_filename = "profile-dump-{0}.csv".format(filename)
    data = csv_to_dict(csv_filename)
    
    extent_range = range(1, max_extent + 1)
    nsamples = len(data[extent_range[0]][y_key])
    x = np.empty((max_extent, nsamples))
    y = np.empty((max_extent, nsamples))
    
    for e in extent_range:
        i = e - 1
        x[i] = data[e][x_key]
        y[i] = data[e][y_key]

    xy = np.array([x, y])
    
    with open(npy_filename, 'w') as f:
        np.save(f, xy)
        
    return xy

def save_regressors(filename, x_key, y_key, max_extent=16):
    """
    Given a filename tag for the profile dump, train all regression models and
    save them to disk. Also persist the profile dump data as an numpy array, if
    not already done.
    """
    xy = save_and_load_xy(filename, x_key, y_key, max_extent=max_extent)
    regressors = fit_xy(xy)
    pickle_filename = "vg-{0}-{x}-{y}.pkl".format(filename, x=x_key, y=y_key)
    joblib.dump(regressors, pickle_filename)

def load_regressors(filename, x_key, y_key):
    pickle_filename = "vg-{0}-{x}-{y}.pkl".format(filename, x=x_key, y=y_key)
    return joblib.load(pickle_filename)


if __name__ == '__main__':
    # Example: python regression.py chrome-ubuntu bandwidth
    import sys

    filename = sys.argv[1]
    x_key = sys.argv[2]
    save_regressors(filename, x_key, 'latency')
