.PHONY: clean main ws_server edf_converter installdeps

CXX = c++

CSRC := storage/EDFlib/edflib.c
CPPSRC := storage/edf_backend.cpp\
					storage/hdf5_backend.cpp\
					compute/helpers.cpp\
					compute/eeg_spectrogram.cpp\
					compute/eeg_change_point.cpp
WSSRC := $(CPPSRC)\
					json11/json11.cpp\
					ws_server.cpp
MAINSRC := $(CPPSRC)\
						main.cpp

CONVERTSRC := $(wildcard storage/*.cpp)
OBJ := $(CSRC:.c=.o)
WSOBJ := $(OBJ) $(WSSRC:.cpp=.o)
MAINOBJ := $(OBJ) $(MAINSRC:.cpp=.o)
CONVERTOBJ := $(OBJ) $(CONVERTSRC:.cpp=.o)

CFLAGS := -Wall\
					-std=c++1y\
					-Wno-deprecated-declarations\
					-Wno-write-strings
LDFLAGS := -lfftw3\
					-rdynamic\
					-lboost_system\
					-lboost_regex\
					-lboost_coroutine\
					-lboost_context\
					-lboost_thread\
					-lcrypto\
					-pthread\
					-L/usr/lib/x86_64-linux-gnu\
					/usr/lib/x86_64-linux-gnu/libhdf5_hl_cpp.a\
					/usr/lib/x86_64-linux-gnu/libhdf5_cpp.a\
					/usr/lib/x86_64-linux-gnu/libhdf5_hl.a\
					/usr/lib/x86_64-linux-gnu/libhdf5.a\
					-lz\
					-ldl

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
	sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install

%.o : %.c
	$(CXX) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

ws_server: $(WSOBJ)
	$(CXX) -o $@ $(WSOBJ) $(LDFLAGS)

main: $(MAINOBJ)
	$(CXX) -o $@ $(MAINOBJ) $(LDFLAGS)

edf_converter: $(CONVERTOBJ)
	$(CXX) -o $@ $(CONVERTOBJ) $(LDFLAGS)

clean:
	find . -type f -name '*.*~' -delete
	find . -type f -name '*.[dSYM|o|d]' -delete
	find . -type f -name ws_server -delete
	find . -type f -name main -delete
	find . -type f -name edf_converter -delete
