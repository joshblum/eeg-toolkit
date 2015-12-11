FROM ubuntu:14.04
MAINTAINER Joshua Blum <joshblum@mit.edu>
ENV DEBIAN_FRONTEND noninteractive    # export DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && apt-get install -y -q \
                                  make
ADD ./webapp/requirements.txt /tmp/requirements.txt
RUN make installdeps
ADD ./webapp /opt/webapp/
WORKDIR /opt/webapp
EXPOSE 5000
CMD ["python", "server.py"]
