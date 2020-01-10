.DELETE_ON_ERROR:

ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CFLAGS += -Wall -Wextra -Wunused

CPPFLAGS += -std=c99

# Look for generated files in the base directory
CPPFLAGS += -I.

ifneq ($(LTO),0)
LTO_FLAGS += -flto=jobserver -fuse-linker-plugin
endif

all: gen listen sine-gen avr-top

sine-gen: AVR_CFLAGS =#  ensure we do not get flags meant for embedded
sine-gen: AVR_LDFLAGS =# ensure we do not get flags meant for embedded
sine-gen: CC = cc#       ensure we do not get compiler meant for embedded
sine-gen: sine.o

SINETABLE_GAIN = 1.0
sinetable_%_.h: sine-gen
	$(realpath $<) $* $(SINETABLE_GAIN) > $@

sine-precomp.o: CPPFLAGS += -I.

avr-%: ARCH_FLAGS = -mmcu=attiny412

avr-%: CC = avr-gcc
avr-%: LD = avr-gcc

AVR_OPTFLAGS ?= -Os $(LTO_FLAGS)

AVR_CFLAGS += $(ARCH_FLAGS)
AVR_CFLAGS += $(AVR_OPTFLAGS)
AVR_CFLAGS += -ffunction-sections
AVR_CFLAGS += -Werror=conversion
AVR_CFLAGS += -fstack-usage
AVR_CFLAGS += -fshort-enums
AVR_CFLAGS += -Wpadded
avr-%.o: CFLAGS += $(AVR_CFLAGS)

AVR_LDFLAGS += $(AVR_OPTFLAGS)
AVR_LDFLAGS += $(ARCH_FLAGS)
AVR_LDFLAGS += -nostdlib
avr-%: LDFLAGS += $(AVR_LDFLAGS)

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

avr-top: avr-sine-precomp.o
avr-top: avr-encode.o
avr-top: avr-decode.o

FLASH_SECTIONS = text data vectors
%.hex: avr-%
	avr-objcopy $(FLASH_SECTIONS:%=-j .%) -O ihex $< $@

gen listen: CFLAGS += -O3

vpath %.c src

gen: encode.o
gen: sine.o
listen: decode.o

coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read a b c ; echo $$a $$b $$c)) > $@

%.d avr-%.d: %.c
	@$(COMPILE.c) -MM -MG -MF $*.d $<
	@$(COMPILE.c) -MM -MG -MT avr-$*.o -MF avr-$*.d $<

-include $(patsubst %.c,%.d,$(notdir $(wildcard src/*.c)))
-include $(patsubst %.c,avr-%.d,$(notdir $(wildcard src/*.c)))

clean:
	rm -f *.d *.o gen listen sine-gen coeffs_*.h sinetable_*.h

