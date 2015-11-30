from flask import Flask
from flask import jsonify
from flask import render_template
from flask import request

import csv
import simplejson as json


app = Flask(__name__)
PROFILE_DUMP_FILENAME = 'profileDump.csv'


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


def dump_profiles(profileDumps):
  profileDumps.sort(key=lambda profile: profile.get('extent', 1))
  with open(PROFILE_DUMP_FILENAME, 'w') as csvfile:
    for profile in profileDumps:
      if 'extent' not in profile:
        profile['extent'] = 1

    fieldnames = sorted(profileDumps[0].keys())
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for profile in profileDumps:
      writer.writerow(profile)


@app.route('/dump-visgoth', methods=['POST'])
def dump_visgoth():
  success = True
  message = ''

  try:
    profileDumps = json.loads(request.form['profileDumps'])
    profileDumps = profileDumps.values()
    if len(profileDumps) > 0:
      dump_profiles(profileDumps)
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
