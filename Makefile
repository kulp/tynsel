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

ifneq ($(ENCODE_BITS),)
CPPFLAGS += -DENCODE_BITS=$(ENCODE_BITS)
endif
ifneq ($(DECODE_BITS),)
CPPFLAGS += -DDECODE_BITS=$(DECODE_BITS)
endif

BIT_VARYING += decode.c
BIT_VARYING += encode.c
BIT_VARYING += sine-gen.c
BIT_VARYING += sine-precomp.c
BIT_VARYING += sine.c

all: gen listen sine-gen avr-top

sine-gen: AVR_CPPFLAGS =#ensure we do not get flags meant for embedded
sine-gen: AVR_CFLAGS =#  ensure we do not get flags meant for embedded
sine-gen: AVR_LDFLAGS =# ensure we do not get flags meant for embedded
sine-gen: CC = cc#       ensure we do not get compiler meant for embedded
sine-gen: sine.o

avr-encode-% encode-%: CPPFLAGS += -DENCODE_BITS=BITWIDTH
avr-decode-% decode-%: CPPFLAGS += -DDECODE_BITS=BITWIDTH

avr-sine-% sine-%: CPPFLAGS += -DENCODE_BITS=BITWIDTH

%-8bit.d  %-8bit.o:  CPPFLAGS += -DBITWIDTH=8
%-16bit.d %-16bit.o: CPPFLAGS += -DBITWIDTH=16

%-8bit.o:  %.c ; $(COMPILE.c) -o $@ $<
%-16bit.o: %.c ; $(COMPILE.c) -o $@ $<
avr-%-8bit.o:  %.c ; $(COMPILE.c) -o $@ $<
avr-%-16bit.o: %.c ; $(COMPILE.c) -o $@ $<

SINETABLE_GAIN = 1.0
sinetable_%_.h: sine-gen
	$(realpath $<) $* $(SINETABLE_GAIN) > $@

-include avr-site.mk

avr-%: ARCH_FLAGS += -mmcu=attiny412

avr-%: CC = avr-gcc
avr-%: LD = avr-gcc

AVR_OPTFLAGS ?= -Os $(LTO_FLAGS)

AVR_CPPFLAGS += $(ARCH_FLAGS)
AVR_CPPFLAGS += -DNDEBUG
avr-%: CPPFLAGS += $(AVR_CPPFLAGS)

AVR_CFLAGS += $(AVR_OPTFLAGS)
AVR_CFLAGS += -ffunction-sections
AVR_CFLAGS += -Werror=conversion
AVR_CFLAGS += -fstack-usage
AVR_CFLAGS += -fshort-enums
AVR_CFLAGS += -Wpadded
avr-%.o: CFLAGS += $(AVR_CFLAGS)

AVR_LDFLAGS += $(AVR_OPTFLAGS)
AVR_LDFLAGS += $(ARCH_FLAGS)
avr-%: LDFLAGS += $(AVR_LDFLAGS)

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

avr-top: avr-sine-precomp-16bit.o
avr-top: avr-encode-16bit.o
avr-top: avr-decode-16bit.o

FLASH_SECTIONS = text data vectors
%.hex: avr-%
	avr-objcopy $(FLASH_SECTIONS:%=-j .%) -O ihex $< $@

gen listen: CFLAGS += -O3

vpath %.c src

gen: encode-16bit.o
gen: sine-16bit.o
listen: decode-16bit.o

coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read a b c ; echo $$a $$b $$c)) > $@

%.d avr-%.d: %.c
	@$(COMPILE.c) -MM -MG -MF $*.d $<
	@$(COMPILE.c) -MM -MG -MT avr-$*.o -MF avr-$*.d $<

%-16bit.d: %.c
	@$(COMPILE.c) -MM -MG -MT $(@:.d=.o) -MF $@ $<

avr-%-16bit.d: %.c
	@$(COMPILE.c) -MM -MG -MT $(@:.d=.o) -MF $@ $<

%-8bit.d: %.c
	@$(COMPILE.c) -MM -MG -MT $(@:.d=.o) -MF $@ $<

avr-%-8bit.d: %.c
	@$(COMPILE.c) -MM -MG -MT $(@:.d=.o) -MF $@ $<

-include $(patsubst %.c,%.d,$(notdir $(wildcard src/*.c)))
-include $(patsubst %.c,avr-%.d,$(notdir $(wildcard src/*.c)))
-include $(patsubst %.c,%-16bit.d,$(BIT_VARYING))
-include $(patsubst %.c,%-8bit.d,$(BIT_VARYING))
-include $(patsubst %.c,avr-%-16bit.d,$(BIT_VARYING))
-include $(patsubst %.c,avr-%-8bit.d,$(BIT_VARYING))

clean:
	rm -f *.d *.o gen listen sine-gen coeffs_*.h sinetable_*.h

