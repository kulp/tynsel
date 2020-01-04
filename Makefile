.DELETE_ON_ERROR:

ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CFLAGS += -Wall -Wextra -Wunused

CPPFLAGS += -std=c99

all: gen listen sine

sine: CPPFLAGS += -DGENERATE_SINE_TABLE
# `sine` binary is built directly from source to avoid erroneously reusing a
# sine.o built elsewhere
sine: sine.c
	$(LINK.c) -o $@ $^

SINETABLE_GAIN = 1.0
sinetable_%_.h: sine
	$(realpath $<) $* $(SINETABLE_GAIN) > $@

sine-precomp.o: CPPFLAGS += -I.

avr-%: ARCH_FLAGS = -mmcu=attiny412

avr-%: CC = avr-gcc
avr-%: LD = avr-gcc

avr-%.o: CFLAGS += -Os $(ARCH_FLAGS)
avr-%.o: CFLAGS += -ffunction-sections
avr-%.o: CFLAGS += -Werror=conversion
avr-%.o: CFLAGS += -fstack-usage

avr-%: LDFLAGS += $(ARCH_FLAGS)
avr-%: LDFLAGS += -nostdlib

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

gen listen: CFLAGS += -O3

vpath %.c src

gen: encode.o
gen: sine.o
listen: decode.o

avr-decode.o decode.o: CPPFLAGS += -I.
coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read a b c ; echo $$a $$b $$c)) > $@

%.d avr-%.d: %.c
	@$(COMPILE.c) -MM -MG -MF $*.d $<
	@$(COMPILE.c) -MM -MG -MT avr-$*.o -MF avr-$*.d $<

-include $(patsubst %.c,%.d,$(notdir $(wildcard src/*.c)))
-include $(patsubst %.c,avr-%.d,$(notdir $(wildcard src/*.c)))

clean:
	rm -f *.d *.o gen listen coeffs_*.h sinetable_*.h

