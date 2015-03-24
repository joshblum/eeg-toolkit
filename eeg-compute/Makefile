CXX = g++
TARGET := main
CSRC := edflib.c
CPPSRC := spectrogram.cpp
OBJ := $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

CFLAGS = -Wall -Wextra -Wshadow -Wformat-nonliteral -Wformat-security -Wtype-limits -g -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE
LDFLAGS = -Wall

default: $(TARGET)

%.o : %.c
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -c $< -o $@

main : main.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -lfftw3 -lm -o $@

clean:
	rm -f -r *.dSYM *.o *.d *~ $(TARGET)
