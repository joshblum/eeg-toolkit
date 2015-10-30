from flask import Flask
from flask import render_template


app = Flask(__name__)


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


if __name__ == '__main__':
  app.debug = True  # enable auto reload
  app.run(host='0.0.0.0')
