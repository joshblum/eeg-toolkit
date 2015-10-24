from __future__ import division

from tornado.ioloop import IOLoop
from tornado.web import Application
from tornado.web import StaticFileHandler
from tornado.web import RequestHandler

from ws_server import SpectrogramWebSocket


class MainHandler(RequestHandler):

  def get(self):
    self.render('html/main.html')


def make_app():
  handlers = [
      (r'/', MainHandler),
      (r'/compute/spectrogram/', SpectrogramWebSocket),
      (r'/(.*)', StaticFileHandler, {
          'path': ''
      }),
  ]
  return Application(handlers, autoreload=True)


def main():
  app = make_app()
  port = 5000
  app.listen(port)
  print 'Listing on port:', port
  IOLoop.current().start()

if __name__ == '__main__':
  main()
