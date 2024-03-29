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

SINETABLE_GAIN = 1.0
sinetable_%_16b.h: sine-gen-16bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@
sinetable_%_8b.h: sine-gen-8bit
	$(realpath $^) $$(echo $* | (IFS=_; read a b ; echo $$a)) $(SINETABLE_GAIN) > $@

-include avr-site.mk

avr-%: CC = $(AVR_CC)
avr-%: LD = $(AVR_CC)

AVR_OPTFLAGS ?= -Os $(LTO_FLAGS)

AVR_CPPFLAGS += $(ARCH_FLAGS)
avr-%: CPPFLAGS += $(AVR_CPPFLAGS)

AVR_CFLAGS += $(AVR_OPTFLAGS)
AVR_CFLAGS += -fshort-enums
AVR_CFLAGS += -Wpadded
avr-%.o: CFLAGS += $(AVR_CFLAGS)

AVR_LDFLAGS += $(AVR_OPTFLAGS)
AVR_LDFLAGS += $(ARCH_FLAGS)
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

FREQUENCIES = $(shell echo 'FREQUENCY_LIST(FLATTEN3)' | avr-cpp -P $(CPPFLAGS) -imacros src/types.h -D'FLATTEN3(X,Y,Z)=Z')
coeffs_%.h: scripts/gen_notch.m
	$(realpath $<) $$(echo $* | (IFS=_; read sample_rate notch_width rest ; echo $$sample_rate $$notch_width)) $(FREQUENCIES) > $@

OBJ_PREFIXES = NULL avr-
OBJ_SUFFIXES = NULL -8bit -16bit

# Generate dependency files.
%.o: CFLAGS += -MMD
-include *.d

ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
$(foreach p,$(PATS),$(eval -include $(patsubst %.c,$p,$(SOURCES))))
endif

clean:
	rm -f *.d *.o gen listen sine-gen-*bit

# The `clobber` rule cleans up generated code, too.
clobber: clean
	rm coeffs_*.h sinetable_*.h
