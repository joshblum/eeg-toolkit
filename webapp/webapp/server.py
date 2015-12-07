from flask import Flask
from flask import jsonify
from flask import render_template
from flask import request

import os

import csv
import simplejson as json


app = Flask(__name__)
PROFILE_FILENAME = 'profile-dump-{0}.csv'


@app.errorhandler(404)
def page_not_found(error):
  return render_template('404.html'), 404


@app.errorhandler(500)
def server_error(error):
  return render_template('500.html'), 500


@app.route('/cluster-viewer')
def cluster_viewer():
  return render_template('cluster-viewer.html')


@app.route('/about')
def about():
  return render_template('about.html')


@app.route('/spec-viewer')
@app.route('/')
def index():
  # TODO(joshblum): put stuff behind a login?
  return render_template('spec-viewer.html')


def dump_profiles(profile_dumps):
  # Figure out what label to give this experiment.
  next_experiment_num = 0
  for filename in os.listdir(os.getcwd()):
    split_filename = filename.split('-')

    if PROFILE_FILENAME.split('-')[:2] == split_filename[:2]:
      try:
        experiment_num = int(split_filename[2].split('.')[0])
      except:
        continue
      if experiment_num >= next_experiment_num:
        next_experiment_num = experiment_num + 1
  next_experiment_num = str(next_experiment_num).zfill(3)
  profile_filename = PROFILE_FILENAME.format(next_experiment_num)

  # Sort the rows by increasing extent.
  profile_dumps.sort(key=lambda profile: profile.get('extent', 1))

  with open(profile_filename, 'w') as csvfile:
    for profile in profile_dumps:
      if 'extent' not in profile:
        profile['extent'] = 1

    # Column order is: extent, then all others alphabetically.
    fieldnames = profile_dumps[0].keys()
    fieldnames.remove('extent')
    fieldnames.sort()
    fieldnames = ['extent'] + fieldnames

    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for profile in profile_dumps:
      # Set default value of -1 for any missing fields.
      for fieldname in fieldnames:
        if fieldname not in profile:
          profile[fieldname] = -1
      writer.writerow(profile)


@app.route('/dump-visgoth', methods=['POST'])
def dump_visgoth():
  success = True
  message = ''

  try:
    profile_dumps = json.loads(request.form['profileDumps'])
    profile_dumps = profile_dumps.values()
    if len(profile_dumps) > 0:
      dump_profiles(profile_dumps)
  except Exception as e:
    success = False
    message = e.message

  return jsonify({
      'success': success,
      'message': message,
  })


if __name__ == '__main__':
  app.debug = True  # enable auto reload
  app.run(host='0.0.0.0')
