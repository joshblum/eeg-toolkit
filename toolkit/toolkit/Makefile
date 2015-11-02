.PHONY: clean main ws_server installdeps

OS := $(shell uname)

CXX = c++

CSRC := storage/EDFlib/edflib.c
CPPSRC := storage/backends.cpp\
					compute/helpers.cpp\
					compute/eeg_spectrogram.cpp\
					compute/eeg_change_point.cpp
WSSRC := $(CPPSRC)\
	json11/json11.cpp\
	ws_server.cpp
MAINSRC := $(CPPSRC)\
		compute/main.cpp

WSOBJ := $(CSRC:.c=.o) $(WSSRC:.cpp=.o)
MAINOBJ := $(CSRC:.c=.o) $(MAINSRC:.cpp=.o)

CFLAGS := -Wall\
					-std=c++1y\
					-Wno-deprecated-declarations
LDFLAGS := -lfftw3\
					-rdynamic\
					-lboost_system\
					-lboost_regex\
					-lboost_coroutine\
					-lboost_context\
					-lboost_thread\
					-lcrypto

ifeq ('$(OS)', 'Darwin')
	LDFLAGS += -L/usr/local/opt/openssl/lib
	CFLAGS += -I/usr/local/opt/openssl/include\
						-Wno-writable-strings
else
	LDFLAGS += -pthread
	CFLAGS += -Wno-write-strings
endif

ifeq ($(DEBUG),1)
	 CFLAGS += -O0\
						 -g\
						 -DDEBUG # -g needed for test framework assertions
else
	CFLAGS += -O3\
						-DNDEBUG
endif

default: ws_server

installdeps:
ifeq ('$(OS)', 'Darwin')
	# Run MacOS commands
	brew update
	cat packages-osx.txt | xargs brew install
else
	# Run Linux commands
	sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install
endif

%.o : %.c
	$(CXX) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

ws_server: $(WSOBJ)
	$(CXX) -o $@ $(WSOBJ) $(LDFLAGS)

main: $(MAINOBJ)
	$(CXX) -o $@ $(MAINOBJ) $(LDFLAGS)

clean:
	find . -type f -name '*.*~' -delete
	find . -type f -name '*.[dSYM|o|d]' -delete
	find . -type f -name ws_server -delete
	find . -type f -name main -delete
