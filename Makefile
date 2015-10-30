.PHONY: clean ws_server run installdeps lint pylint jslint libs prod-run install deploy submodules submodule-update dev-packages

CXX = c++
CSRC := compute/EDFlib/edflib.c
CPPSRC := json11/json11.cpp compute/helpers.cpp compute/edf_backend.cpp compute/eeg_spectrogram.cpp\
	compute/eeg_change_point.cpp ws_server.cpp
OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)
TARGET := ws_server
CFLAGS = -Wall -std=c++1y -Wno-deprecated-declarations
LDFLAGS = -lfftw3 -rdynamic -lboost_system -lboost_regex -lcrypto
OS := $(shell uname)

ifeq ('$(OS)', 'Darwin')
	OSX = true
	LDFLAGS += -L/usr/local/opt/openssl/lib
	CFLAGS += -I/usr/local/opt/openssl/include -Wno-writable-strings
else
	LDFLAGS += -pthread -lboost_thread
	CFLAGS += -Wno-write-strings
endif

ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g -DDEBUG # -g needed for test framework assertions
else
	CFLAGS += -O3 -DNDEBUG
endif

default: ws_server

libs:
	make -C compute/ libs

%.o : %.c
	$(CXX) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

ws_server: $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

submodule-update:
	git submodule foreach git checkout master; git pull

submodules:
	git submodule update --init --recursive

installdeps: clean submodules
ifeq ('$(OSX)', 'true')
	# Run MacOS commands
	brew update
	cat packages-osx.txt | xargs brew install
else
	# Run Linux commands
	-sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install
endif
	pip install -r requirements.txt

dev-packages:
ifeq ('$(OSX)', 'true')
	cat packages-dev-osx.txt | xargs brew install
else
	# Run Linux commands
	-sudo apt-get update
	cat packages-dev.txt | xargs sudo apt-get -y install
endif


install: installdeps ws_server

deploy:
	fab prod deploy

run: ws_server
	./ws_server 8080 & \
		python server.py

prod-run: clean ws_server
	supervisorctl reread
	supervisorctl update
	supervisorctl restart eeg:eeg
	supervisorctl restart ws:ws

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

clean:
	find . -type f -name '*.py[cod]' -delete
	find . -type f -name '*.*~' -delete
	find . -type f -name $(TARGET) -delete
	find . -type f -name '*.[dSYM|o|d]' -delete
