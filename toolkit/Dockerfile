FROM ubuntu:14.04
MAINTAINER Joshua Blum <joshblum@mit.edu>
RUN apt-get update && apt-get install -y -q \
                            make \
                            g++
ADD ./toolkit /opt/toolkit/
WORKDIR /opt/toolkit
RUN make installdeps
# create binaries and run daemons
RUN start.sh
EXPOSE 8080
CMD ["./ws_server"]
