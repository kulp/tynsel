.DELETE_ON_ERROR:

ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINE))

CFLAGS += -Wall -Wextra -Wunused

CPPFLAGS += -std=c99

all: gen wrap decode

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

decode: CFLAGS += -O3

wrap: CFLAGS += -Os -fomit-frame-pointer

INCLUDE += src src/recognisers
vpath %.c src src/recognisers

CPPFLAGS += $(patsubst %,-I%,$(INCLUDE))

gen: LDLIBS += -lsndfile
gen: encode.o audio.o

avr-decode.o decode.o: INCLUDE += .
coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read a b c ; echo $$a $$b $$c)) > $@

%.d: %.c
	@$(COMPILE.c) -MM -MG -MF $@ $<

-include $(patsubst %.c,%.d,$(notdir $(wildcard *.c src/recognisers/*.c src/*.c)))

clean:
	rm -f *.d *.o gen wrap decode

