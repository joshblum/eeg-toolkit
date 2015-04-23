ROOT := $(dir $(lastword $(MAKEFILE_LIST)))

CXX = g++
TARGET := main
SHARED_LIB = $(addprefix $(ROOT), lib_eeg_spectrogram.so)
CSRC := $(addprefix $(ROOT), edflib.c)
CPPSRC := $(addprefix $(ROOT), eeg_spectrogram.cpp)

OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

CFLAGS = -Wall -Wextra -Wshadow -Wformat-nonliteral -Wformat-security -Wtype-limits -fPIC -std=c++11 -g -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE

ifeq ($(DEBUG),1)
	 CFLAGS += -O0 -g -DDEBUG # -g needed for test framework assertions
else
	CFLAGS += -O3 -DNDEBUG $(PFLAG)
endif

LDFLAGS = -Wall

%.o : %.c
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

libs: $(OBJ)
	$(CXX) $(CFLAGS) $(LDFLAGS) -lfftw3 -lm -shared -o $(SHARED_LIB) $(OBJ)
main: main.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -lfftw3 -lm -o $@

clean:
	rm -f -r *.dSYM *.o *.d *~ $(TARGET) $(SHARED_LIB)
