FROM ubuntu:14.04
MAINTAINER Joshua Blum <joshblum@mit.edu>
RUN apt-get update && apt-get install -y -q \
                            libarmadillo-dev \
                            libboost-all-dev \
                            libfftw3-dev \
                            libssl-dev \
                            make \
                            g++
ADD ./toolkit /opt/toolkit/
WORKDIR /opt/toolkit
RUN make ws_server
EXPOSE 8080
CMD ["./ws_server"]
