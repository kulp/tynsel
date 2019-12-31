ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINE))

CFLAGS += -Wall -Wextra -Wunused

CPPFLAGS += -std=c99

all: suite gen wrap

avr-%: ARCH_FLAGS += -mmcu=attiny412

avr-%: CC = avr-gcc
avr-%: LD = avr-gcc
avr-%: CFLAGS += -Os $(ARCH_FLAGS)
avr-%: LDFLAGS += $(ARCH_FLAGS)
avr-%: CFLAGS += -ffunction-sections
avr-%: CFLAGS += -Werror=conversion
avr-%: LDFLAGS += -nostdlib
avr-%: CFLAGS += -fstack-usage

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

top: CFLAGS += -O3

top.o: CPPFLAGS += -D$*_main=main

top.o: decode.c
	$(COMPILE.c) -o $@ $<

wrap: CFLAGS += -Os -fomit-frame-pointer

INCLUDE += src src/recognisers
vpath %.c src src/recognisers

CPPFLAGS += $(patsubst %,-I%,$(INCLUDE))

gen: LDLIBS += -lsndfile
gen: encode.o audio.o

suite: LDLIBS += -lsndfile
suite: filters.o streamdecode.o audio.o

%.d: %.c
	@$(COMPILE.c) -MM -MG -MF $@ $<

-include $(patsubst %.c,%.d,$(notdir $(wildcard *.c src/recognisers/*.c src/*.c)))

clean:
	rm -f *.d *.o gen suite wrap

