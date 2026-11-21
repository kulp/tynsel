.DELETE_ON_ERROR:

SAMPLE_RATE = 8000

TARGETS += generic

AVR_CC = avr-gcc
HAVE_AVR_GCC := $(if $(shell /bin/sh -c "command -v $(AVR_CC)"),1)
$(if $(HAVE_AVR_GCC),$(eval TARGETS += avr-top),$(warning No AVR compiler `$(AVR_CC)` was found; skipping AVR compilation))

ifneq ($(DEBUG),)
CFLAGS += -g -O0
CXXFLAGS += -g -O0
LDFLAGS += -g -O0
CPPFLAGS += -DDEBUG
else
CPPFLAGS += -DNDEBUG
endif

CFLAGS += -Wall -Wextra -Wunused
CFLAGS += -Wc++-compat -Wno-error=c++-compat
CFLAGS += $(if $(WERROR),-Werror,-Wno-error)

CPPFLAGS += -std=c11

# Look for generated files in the base directory
coeff.o sine-precomp%.o avr-coeff.o avr-sine-precomp%.o: CPPFLAGS += -I.

ifneq ($(LTO),0)
LTO_FLAGS += -flto
endif

CPPFLAGS += $(if $(ENCODE_BITS),-DENCODE_BITS=$(ENCODE_BITS))
CPPFLAGS += $(if $(DECODE_BITS),-DDECODE_BITS=$(DECODE_BITS))

CPPFLAGS += -DSAMPLE_RATE=$(SAMPLE_RATE)

SOURCES = $(notdir $(wildcard src/*.c))

all: $(TARGETS)

.PHONY: test
test: all

test: test-gen-decode
.PHONY: test-gen-decode
test-gen-decode:
	./scripts/test-gen-decode.sh

test: test-start-bit
.PHONY: test-start-bit
test-start-bit:
	./scripts/test-start-bit.sh

test: test-coeffs
.PHONY: test-coeffs
test-coeffs:
	./scripts/test-coeffs.sh

# The `generic` target builds things that need no special hardware.
generic: gen listen sine-gen-8bit sine-gen-16bit

sine-gen%: AVR_CPPFLAGS =#ensure we do not get flags meant for embedded
sine-gen%: AVR_CFLAGS =#  ensure we do not get flags meant for embedded
sine-gen%: AVR_LDFLAGS =# ensure we do not get flags meant for embedded
sine-gen%: CC = cc#       ensure we do not get compiler meant for embedded
sine-gen-%: LDLIBS += -lm
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

.SECONDARY: sine-gen-8bit.o sine-gen-16bit.o
SINETABLE_GAIN = 1.0
sinetable_%_16b.h: sine-gen-16bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@
sinetable_%_8b.h: sine-gen-8bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@

-include avr-site.mk

avr-%: CC = $(AVR_CC)
avr-%: LD = $(AVR_CC)

AVR_OPTFLAGS ?= -Os $(LTO_FLAGS)

# The microcontroller the embedded build is for. avr-site.mk or the command line
# can override it. Without -mmcu, avr/io.h defines no DAC0, ADC0, or GPIOR0, so
# src/avr-top.c would not compile at all; the part named here has to offer both
# a DAC, which modulates the audio, and an ADC, which clocks the samples.
F_MCU ?= attiny412
AVR_ARCH_FLAGS ?= -mmcu=$(F_MCU)

AVR_CPPFLAGS += $(ARCH_FLAGS) $(AVR_ARCH_FLAGS)
avr-%: CPPFLAGS += $(AVR_CPPFLAGS)

AVR_CFLAGS += $(AVR_OPTFLAGS)
AVR_CFLAGS += -fshort-enums
AVR_CFLAGS += -Wpadded
avr-%.o: CFLAGS += $(AVR_CFLAGS)

AVR_LDFLAGS += $(AVR_OPTFLAGS)
AVR_LDFLAGS += $(ARCH_FLAGS) $(AVR_ARCH_FLAGS)
avr-%: LDFLAGS += $(AVR_LDFLAGS)

avr-%.o: %.c
	$(COMPILE.c) -o $@ $<

.SECONDEXPANSION:
avr-top: ENCODE_BITS = 8
avr-top: DECODE_BITS = 16
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
gen: LDLIBS += -lm
listen: decode-16bit.o
listen: decode-8bit.o
listen: decode-heap-16bit.o
listen: decode-heap-8bit.o
listen: coeff.o

# The frequencies come from FREQUENCY_LIST in src/types.h. avr-cpp would do the
# same job, but a host without the AVR toolchain installed would then regenerate
# this header with no frequencies at all, and the redirection would leave an
# empty file behind that the compiler only reports on later.
FREQUENCIES = $(shell echo 'FREQUENCY_LIST(FLATTEN3)' | $(CC) -E -P -x c - $(CPPFLAGS) -imacros src/types.h -D'FLATTEN3(X,Y,Z)=Z')
# A fresh checkout writes coeffs_8000_150_.h before scripts/gen_notch.m, so
# the generator always timestamps newer than the checked-in header. With the
# generator as a plain prerequisite, the second `make` in a clean clone (CI
# runs `make all` and then `make test`) regenerates the header, which needs
# Octave, and hosts without it fail the build with `octave-cli: No such file
# or directory`. The header is checked in precisely so that an ordinary build
# needs no Octave, so make the prerequisite conditional; refresh the header
# deliberately with `make REGEN_COEFFS=1 coeffs_8000_150_.h` after editing
# gen_notch.m or the FREQUENCY_LIST in src/types.h. The recipe names the
# generator by path rather than through `$<`, because after `make clobber`
# the header is missing and must be regenerated with REGEN_COEFFS unset.
coeffs_%.h: $(if $(REGEN_COEFFS),scripts/gen_notch.m)
	$(realpath scripts/gen_notch.m) $$(echo $* | (IFS=_; read sample_rate notch_width rest ; echo $$sample_rate $$notch_width)) $(FREQUENCIES) > $@

# Generate dependency files as objects are compiled, and read them back on the
# next run. The block that used to iterate over $(PATS) never ran: PATS was
# never defined, and neither were OBJ_PREFIXES, OBJ_SUFFIXES, or SOURCES, which
# it consumed.
%.o: CFLAGS += -MMD
-include *.d

clean:
	rm -f *.d *.o gen listen sine-gen-*bit

# The `clobber` rule cleans up generated code, too.
clobber: clean
	rm coeffs_*.h sinetable_*.h
