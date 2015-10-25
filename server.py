from flask import Flask
from flask import send_from_directory


app = Flask(__name__)


@app.route('/img/<path:path>')
def send_img(path):
  return send_from_directory('img', path)


@app.route('/css/<path:path>')
def send_css(path):
  return send_from_directory('css', path)


@app.route('/js/<path:path>')
def send_js(path):
  return send_from_directory('js', path)


@app.route('/')
def main():
  return send_from_directory('html', 'main.html')


if __name__ == '__main__':
  app.run(host='0.0.0.0')
