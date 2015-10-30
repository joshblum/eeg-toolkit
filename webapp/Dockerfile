FROM ubuntu:14.04
MAINTAINER Joshua Blum <joshblum@mit.edu>
RUN apt-get update && apt-get install -y -q \
                                  python-all \
                                  python-pip \
                                  python-dev
ADD ./webapp/requirements.txt /tmp/requirements.txt
RUN pip install -r /tmp/requirements.txt
ADD ./webapp /opt/webapp/
WORKDIR /opt/webapp
EXPOSE 5000
CMD ["python", "server.py"]
