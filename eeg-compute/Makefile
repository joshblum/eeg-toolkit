CXX = g++
TARGET := main
CSRC := edflib.c
CPPSRC := spectrogram.cpp
OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

CFLAGS = -Wall -Wextra -Wshadow -Wformat-nonliteral -Wformat-security -Wtype-limits -g -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE

ifeq ($(DEBUG),1)
	 CFLAGS += -O0 -g -DDEBUG # -g needed for test framework assertions
else
	CFLAGS += -O3 -DNDEBUG $(PFLAG)
endif

LDFLAGS = -Wall

default: spectrogram

%.o : %.c
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

spectrogram:
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -ansi -c edflib.c spectrogram.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -lfftw3 -lm -shared edflib.o spectrogram.o -o libspectrogram.so

main: main.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -lfftw3 -lm -o $@

clean:
	rm -f -r *.dSYM *.o *.d *~ $(TARGET)
