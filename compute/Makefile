.PHONY: libs main clean

ROOT := $(dir $(lastword $(MAKEFILE_LIST)))
OS := $(shell uname)

CXX = g++
TARGET := main
SHARED_LIB = $(addprefix $(ROOT), lib_eeg_spectrogram.so)
CSRC := $(addprefix $(ROOT), edflib.c)
CPPSRC := $(addprefix $(ROOT),  eeg_spectrogram.cpp eeg_change_point.cpp)

OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

CFLAGS = -Wall -Wextra -Wshadow -Wformat-nonliteral -Wformat-security\
				 -Wtype-limits -fPIC -std=c++11 \
				 -Wno-writable-strings\
				 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE\
	
ifeq ('$(OS)', 'Darwin')
	OSX = true
	CFLAGS += -Wno-c++11-compat-deprecated-writable-strings
else
	CFLAGS += -Wno-write-strings
endif

ifeq ($(DEBUG),1)
	 CFLAGS += -O0 -g -DDEBUG # -g needed for test framework assertions
ifeq ('(OSX)', 'true')
	CFLAGS += -gdwarf-2
endif
else
	CFLAGS += -O3 -DNDEBUG $(PFLAG)
endif

LDFLAGS = -Wall

default: libs

%.o : %.c
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

libs: $(OBJ)
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -o $(SHARED_LIB) $(OBJ) -lfftw3

main: main.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -lfftw3 -lm -o $@

clean:
	find . -type f -name '*.[dSYM|o|d]' -delete
	find . -type f -wholename '[$(TARGET)|$(SHARED_LIB)]' -delete
