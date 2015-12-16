FROM ubuntu:14.04
MAINTAINER Joshua Blum <joshblum@mit.edu>
ENV DEBIAN_FRONTEND noninteractive    # export DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && apt-get install -y -q \
                                  make
ADD ./webapp /opt/webapp/
WORKDIR /opt/webapp
RUN make installdeps
EXPOSE 5000
CMD ["python", "server.py"]
