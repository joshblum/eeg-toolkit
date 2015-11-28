.PHONY: ws_server\
	installdeps\
	test\
	edf_converter\
	viz_converter\
	precompute_spectrogram\
	clean

CXX = c++

CSRC := storage/EDFlib/edflib.c
STORAGESRC := storage/binary_backend.cpp\
					storage/edf_backend.cpp\
					storage/hdf5_backend.cpp\
					storage/tiledb_backend.cpp\
					json11/json11.cpp # required by binary_backend for metadata
COMPUTESRC := compute/eeg_spectrogram.cpp\
					compute/eeg_change_point.cpp
VISGOTHSRC := visgoth/visgoth.cpp
TILEDBLIBPATH := storage/TileDB/core/lib/release
TILEDBLIBNAME := libtiledb.so

WSSRC := $(COMPUTESRC)\
				$(STORAGESRC)\
				$(VISGOTHSRC)\
				ws_server.cpp
TESTSRC := $(COMPUTESRC)\
					$(STORAGESRC)\
					test.cpp
EDFCONVERTSRC := $(STORAGESRC)\
								storage/edf_converter.cpp
VIZCONVERTSRC := $(STORAGESRC)\
								compute/eeg_spectrogram.cpp\
								storage/viz_converter.cpp
PRECOMPUTESRC := $(COMPUTESRC)\
								$(STORAGESRC)\
								compute/precompute_spectrogram.cpp

COBJ := $(CSRC:.c=.o)
WSOBJ := $(COBJ) $(WSSRC:.cpp=.o)
TESTOBJ := $(COBJ) $(TESTSRC:.cpp=.o)
EDFCONVERTOBJ := $(COBJ) $(EDFCONVERTSRC:.cpp=.o)
VIZCONVERTOBJ := $(COBJ) $(VIZCONVERTSRC:.cpp=.o)
PREOCMPUTEOBJ := $(COBJ) $(PRECOMPUTESRC:.cpp=.o)

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
					-ldl\
					-L$(TILEDBLIBPATH)\
					-l:$(TILEDBLIBNAME)

ifeq ($(DEBUG),1)
	# -g needed for test framework assertions
	CFLAGS += -O0\
						 -g
	 OPTS += -DDEBUG
else
	CFLAGS += -O3
	OPTS += -DNDEBUG
endif

ifneq ($(BACKEND),)
	OPTS += -DBACKEND=$(BACKEND)
endif

ifneq ($(DATADIR),)
	OPTS += -DDATADIR=$(DATADIR)
endif

ifneq ($(READ_CHUNK_SIZE),)
	OPTS += -DREAD_CHUNK_SIZE=$(READ_CHUNK_SIZE)
endif

ifneq ($(WRITE_CHUNK_SIZE),)
	OPTS += -DWRITE_CHUNK_SIZE=$(WRITE_CHUNK_SIZE)
endif

default: ws_server

installdeps:
	sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install
	pip install -r requirements.txt

%.o : %.c
	$(CXX) $(OPTS) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(OPTS) $(CFLAGS) -o $@ -c $<

ws_server: $(WSOBJ)
	$(CXX) -o $@ $(WSOBJ) $(LDFLAGS)

test: $(TESTOBJ)
	$(CXX) -o $@ $(TESTOBJ) $(LDFLAGS)

edf_converter: $(EDFCONVERTOBJ)
	$(CXX) -o $@ $(EDFCONVERTOBJ) $(LDFLAGS)

viz_converter: $(VIZCONVERTOBJ)
	$(CXX) -o $@ $(VIZCONVERTOBJ) $(LDFLAGS)

precompute_spectrogram: $(PREOCMPUTEOBJ)
	$(CXX) -o $@ $(PREOCMPUTEOBJ) $(LDFLAGS)

clean:
	find . -type f -name '*.*~' -delete
	find . -type f -name '*.[dSYM|o|d]' -delete
	find . -type f -name ws_server -delete
	find . -type f -name test -delete
	find . -type f -name edf_converter -delete
	find . -type f -name viz_converter -delete
	find . -type f -name precompute_spectrogram -delete
