FROM ubuntu:14.04
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
MAINTAINER Joshua Blum <joshblum@mit.edu>
ENV DEBIAN_FRONTEND noninteractive    # export DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && apt-get install -y -q \
                            make \
                            g++
ADD ./toolkit /home/ubuntu/eeg-toolkit/toolkit/
WORKDIR /home/ubuntu/eeg-toolkit/toolkit/
RUN make installdeps
# create binaries and run daemons
RUN ./docker_build.sh
EXPOSE 8080
ENV LD_LIBRARY_PATH=storage/TileDB/core/lib/release
RUN python file_watcher.py &
CMD ["./docker_start.sh"]
