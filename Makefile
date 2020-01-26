.DELETE_ON_ERROR:

SAMPLE_RATE = 8000

ifneq ($(DEBUG),)
CFLAGS += -g -O0
CXXFLAGS += -g -O0
LDFLAGS += -g -O0
CPPFLAGS += -DDEBUG
else
CPPFLAGS += -DNDEBUG
endif

CFLAGS += -Wall -Wextra -Wunused
CFLAGS += -Wc++-compat
CFLAGS += -Werror

LANG_STD = -std=c11
CPPFLAGS += $(LANG_STD)

# Look for generated files in the base directory
coeff.o sine-precomp%.o avr-coeff.o avr-sine-precomp%.o: CPPFLAGS += -I.

ifneq ($(LTO),0)
LTO_FLAGS += -flto
endif

CPPFLAGS += $(if $(ENCODE_BITS),-DENCODE_BITS=$(ENCODE_BITS))
CPPFLAGS += $(if $(DECODE_BITS),-DDECODE_BITS=$(DECODE_BITS))

CPPFLAGS += -DSAMPLE_RATE=$(SAMPLE_RATE)

SOURCES = $(notdir $(wildcard src/*.c))

all: gen listen avr-top

sine-gen%: AVR_CPPFLAGS =#ensure we do not get flags meant for embedded
sine-gen%: AVR_CFLAGS =#  ensure we do not get flags meant for embedded
sine-gen%: AVR_LDFLAGS =# ensure we do not get flags meant for embedded
sine-gen%: CC = cc#       ensure we do not get compiler meant for embedded
sine-gen-%: sine-gen-%.o sine-%.o
	$(LINK.c) -o $@ $^ $(LDLIBS)

avr-encode-% encode-%: ENCODE_BITS = $(BITWIDTH)
avr-decode-% decode-%: DECODE_BITS = $(BITWIDTH)

avr-sine-% sine-%: ENCODE_BITS = $(BITWIDTH)

%-8bit.d  %-8bit.o:  BITWIDTH = 8
%-16bit.d %-16bit.o: BITWIDTH = 16

%-8bit.o:  %.c ; $(COMPILE.c) -o $@ $<
%-16bit.o: %.c ; $(COMPILE.c) -o $@ $<
avr-%-8bit.o:  %.c ; $(COMPILE.c) -o $@ $<
avr-%-16bit.o: %.c ; $(COMPILE.c) -o $@ $<

avr-coeff% coeff%: CPPFLAGS += -DNOTCH_WIDTH=150

SINETABLE_GAIN = 1.0
sinetable_%_16b.h: sine-gen-16bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@
sinetable_%_8b.h: sine-gen-8bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@

-include avr-site.mk

avr-%: CC = avr-gcc
avr-%: LD = avr-gcc

AVR_OPTFLAGS ?= -Os $(LTO_FLAGS)

AVR_CPPFLAGS += $(ARCH_FLAGS)
avr-%: CPPFLAGS += $(AVR_CPPFLAGS)
avr-%: CPPFLAGS += -DEXTERN=extern

AVR_CFLAGS += $(AVR_OPTFLAGS)
AVR_CFLAGS += -fshort-enums
AVR_CFLAGS += -Wpadded
avr-%.o: CFLAGS += $(AVR_CFLAGS)

AVR_LDFLAGS += $(AVR_OPTFLAGS)
AVR_LDFLAGS += $(ARCH_FLAGS)
avr-%: LDFLAGS += $(AVR_LDFLAGS)

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

sim-%: CXXFLAGS += $(AVR_CFLAGS)
sim-%: LDFLAGS += $(AVR_LDFLAGS)

sim-%: CPPFLAGS += $(AVR_CPPFLAGS)
sim-%: CPPFLAGS += -DNULL=0
sim-%: CPPFLAGS += -Wno-error=unused-command-line-argument
sim-%: CPPFLAGS += -Wno-error=\#warnings
sim-%: CPPFLAGS += -Wno-error=unknown-attributes
sim-%: CPPFLAGS += -Wno-error=macro-redefined
sim-%: CPPFLAGS += -Wno-error=padded

sim-%: CPPFLAGS += -D__AVR_ATtiny412__
# Prevent the use of inline AVR assembly in avr/sleep.h
sim-%: CPPFLAGS += -D_AVR_SLEEP_H_
#sim-%: CPPFLAGS += -D__externally_visible__='visibility("default")'
# Work around incompatible section attributes across object formats
sim-%: CPPFLAGS += -D'section(...)='
sim-%: CPPFLAGS += -D'EXTERN=extern "C"'

#sim-%: CPPFLAGS += -include signal.h
sim-%: CPPFLAGS += -D'sleep_mode()=(void)0' # TODO

# Simulation objects get compiled as C++ files, enabling some dirty tricks to
# replacement assignments
sim-top.o: LANG_STD = -std=c++11

sim-%.o: avr-%.c
	$(COMPILE.cc) -x c++ $(CFLAGS) -o $@ $<

.SECONDEXPANSION:
%-top: ENCODE_BITS = 8
%-top: DECODE_BITS = 16
avr-top: avr-sine-precomp-$$(ENCODE_BITS)bit.o
avr-top: avr-encode-$$(ENCODE_BITS)bit.o
avr-top: avr-decode-$$(DECODE_BITS)bit.o
avr-top: avr-decode-static-$$(DECODE_BITS)bit.o
avr-top: avr-coeff.o

FLASH_SECTIONS = text data vectors
%.hex: avr-%
	avr-objcopy $(FLASH_SECTIONS:%=-j .%) -O ihex $< $@

ifneq ($(DEBUG),)
gen listen: CFLAGS += -O3
endif

vpath %.c src

gen: encode-16bit.o
gen: encode-8bit.o
gen: sine-16bit.o
gen: sine-8bit.o
listen: decode-16bit.o
listen: decode-8bit.o
listen: decode-heap-16bit.o
listen: decode-heap-8bit.o
listen: coeff.o

FREQUENCIES = $(shell echo 'FREQUENCY_LIST(FLATTEN3)' | avr-cpp -P -DSAMPLE_RATE=$(SAMPLE_RATE) -imacros src/types.h -D'FLATTEN3(X,Y,Z)=Z')
coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read sample_rate notch_width rest ; echo $$sample_rate $$notch_width)) $(FREQUENCIES) > $@

OBJ_PREFIXES = NULL avr-
OBJ_SUFFIXES = NULL -8bit -16bit

PATS = $(subst NULL,,$(foreach p,$(OBJ_PREFIXES),$(foreach s,$(OBJ_SUFFIXES),$p%$s.d)))
$(PATS): %.c
	@$(COMPILE.c) -MM -MG -MT "$@ $(@:.d=.o)" -MF $@ $<

ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
$(foreach p,$(PATS),$(eval -include $(patsubst %.c,$p,$(SOURCES))))
endif

clean:
	rm -f *.d *.o gen listen sine-gen-*bit coeffs_*.h sinetable_*.h

