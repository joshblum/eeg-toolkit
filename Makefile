.PHONY: clean jsoncpp server run installdeps lint pylint jslint prod-run install deploy

OS := $(shell uname)

ifeq ($(RPM),1)
	PKG_INSTALLER = yum
else
	PKG_INSTALLER = apt-get
endif

ifeq ('$(OS)', 'Darwin')
	OSX = true
	PKG_INSTALLER = brew
endif

# use `libs` directive for building shared library
-include compute/Makefile

clean:
	find . -type f -name '*.py[cod]' -delete
	find . -type f -name '*.*~' -delete
	rm -f -r *.dSYM *.o *.d *~ server

jsoncpp:
	cd jsoncpp && mkdir -p build\
		&& cd build && cmake -DJSONCPP_LIB_BUILD_STATIC=ON -DJSONCPP_LIB_BUILD_SHARED=OFF -G "Unix Makefiles" ../\
		&& make && make install

CXX = g++
CSRC := compute/edflib.c
CPPSRC := json11/json11.cpp compute/eeg_spectrogram.cpp server.cpp
OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)
CFLAGS = -Wall -std=c++1y -Wno-deprecated-declarations
LDFLAGS = -lboost_system -lcrypto -lfftw3 -lm

ifeq ($(DEBUG),1)
	 CFLAGS += -O0 -g -DDEBUG # -g needed for test framework assertions
else
	CFLAGS += -O3 -DNDEBUG
endif

%.o : %.c
	$(CXX) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

server: $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

run: clean libs
	python server.py

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

installdeps: clean
ifeq ('$(OSX)', 'true')
	# Run MacOS commands
	brew update
	cat packages-osx.txt | xargs brew install
	export PKG_CONFIG_PATH=/usr/local/Cellar/libffi/3.0.13/lib/pkgconfig/
else
	# Run Linux commands
	# setup gcc 4.9
	# TODO(joshblum): setup for yum as well
ifeq ('$(PKG_INSTALLER)', 'apt-get')
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get install -y gcc-4.9
	sudo apt-get install -y g++-4.9
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 20
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 20
	sudo update-alternatives --config gcc
	sudo update-alternatives --config g++
else
	sudo $(PKG_INSTALLER) update
endif
	cat packages.txt | xargs sudo $(PKG_INSTALLER) -y install
endif
	pip install -r requirements.txt
	# start the supervisor daemon
	-supervisord

install: installdeps libs

deploy:
	fab prod deploy

prod-run: libs
	supervisorctl reread
	supervisorctl update
	supervisorctl restart eeg:eeg-8000
