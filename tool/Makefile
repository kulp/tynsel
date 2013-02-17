ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINE))
CPPFLAGS += $(patsubst %,-I%,$(INCLUDE))

CFLAGS += -Wall -Wextra -Wunused

all: correlate

vpath %.c ../src

correlate: CPPFLAGS += -std=c99
correlate: LDLIBS += -lfftw3
correlate: LDLIBS += -lsndfile
correlate: INCLUDE += ../src
correlate: io.o

clean:
	rm -f *.o correlate

