ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CFLAGS += -Wall -Wextra -Wunused

fft: CPPFLAGS += -std=c99
fft: LDLIBS += -lfftw3
g711 gen fft: LDLIBS += -lsndfile

all: fft gen

clean:
	rm -f *.o fft gen
