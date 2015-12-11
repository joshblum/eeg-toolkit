
# coding: utf-8

# In[132]:

from collections import defaultdict
import csv
import math

import numpy as np
from sklearn import linear_model
from sklearn.externals import joblib


CHANGE_BASIS = True
transform_fn = lambda x: 1.0/x
transform_vectorized = np.vectorize(transform_fn)

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

def fit(x, y, extent, test_set_size=None):
    if test_set_size is None:
        x_train, x_test = x, x
        y_train, y_test = y, y
    else:
        x_train, x_test = x[:-test_set_size], x[-test_set_size:]
        y_train, y_test = y[:-test_set_size], y[-test_set_size:]

    if CHANGE_BASIS:
        x_test_transform = transform_vectorized(x_test)
        x_train = transform_vectorized(x_train)

    regr = linear_model.TheilSenRegressor()
    regr.fit(x_train, y_train)
    print extent, regr.score(x_test_transform, y_test)
    
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
    print "Scores for the regressors are:"
    for i in range(nmodels):
        extent = i + 1        
        x_col = x[i, :].reshape((nsamples, 1))
        y_col = y[i, :]
        regrs[extent] = fit(x_col, y_col, extent, test_set_size=25)
    
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


def save_and_load_regressors(filename, x_key, y_key, max_extent=16):
    """
    Given a filename tag for the profile dump, load the corresponding regression models from disk.

    If they don't exist yet, train all regression models and save them to disk.
    Also persist the profile dump data as an numpy array, if not already done.
    """
    import os
    pickle_filename = "vg-{0}-{x}-{y}.pkl".format(filename, x=x_key, y=y_key)
    # If we already serialized the numpy array.
    if os.path.exists(pickle_filename):
        return joblib.load(pickle_filename)

    xy = save_and_load_xy(filename, x_key, y_key, max_extent)
    regressors = fit_xy(xy)
    joblib.dump(regressors, pickle_filename)
    return regressors


def predict_extent(regressors, target_latency, x_value):
    x_value = transform_fn(x_value)
    predictions = []
    for extent, regressor in regressors.items():
        predicted_latency = regressor.predict(x_value)[0]
        distance = abs(predicted_latency - target_latency)
        predictions.append((extent, distance))

    # Get the extent whose regressor predicted the closest to the target.
    optimal = min(predictions, key=lambda prediction: prediction[1])
    extent = optimal[0]
    return extent


if __name__ == '__main__':
    # Example: python regression.py chrome-ubuntu bandwidth
    import sys

    filename = sys.argv[1]
    x_key = sys.argv[2]
    save_and_load_regressors(filename, x_key, 'latency')
