ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINE))

CFLAGS += -Wall -Wextra -Wunused

gen fft: CPPFLAGS += -std=c99
fft: LDLIBS += -lfftw3
g711 gen fft: LDLIBS += -lsndfile

all: fft gen sip

.PHONY: test
test: gen fft
	./test.pl

INCLUDE += src src/recognisers
vpath %.c src src/recognisers

DECODERS = \
           naive.c \
           higher.c \
           correlate.c \
           gsl.c \
           #

CPPFLAGS += $(patsubst %,-I%,$(INCLUDE))

fft: decode.o $(DECODERS) decoders.c
gen: encode.o
fft gen: audio.o
fft: LDLIBS += $(shell pkg-config --libs gsl)

# pjtarget gives us the TARGET_NAME for linking
pjtarget: LDLIBS =
pjtarget: CPPFLAGS =
sip: | pjtarget

# If TARGET_NAME hasn't been set, reinvoke make to get the dependency ordering
# right.
ifeq ($(TARGET_NAME),)
.PHONY: sip
sip:
	$(MAKE) $@ TARGET_NAME="$(shell ./pjtarget)"
endif

sip: LDLIBS_Darwin += \
                      -framework CoreAudio \
                      -framework CoreServices \
                      -framework AudioUnit \
                      -framework AudioToolbox \
                      #

sip: LDLIBS_Linux +=  \
                      -lrt \
                      -lasound \
                      #

sip: LDLIBS := 	\
                -lpjsua-$(TARGET_NAME) \
                -lpjsip-ua-$(TARGET_NAME) \
                -lpjsip-simple-$(TARGET_NAME) \
                -lpjsip-$(TARGET_NAME) \
                -lpjmedia-codec-$(TARGET_NAME) \
                -lpjmedia-$(TARGET_NAME) \
                -lpjmedia-audiodev-$(TARGET_NAME) \
                -lpjnath-$(TARGET_NAME) \
                -lpjlib-util-$(TARGET_NAME) \
                -lresample-$(TARGET_NAME) \
                -lsrtp-$(TARGET_NAME) \
                -lgsmcodec-$(TARGET_NAME) \
                -lspeex-$(TARGET_NAME) \
                -lilbccodec-$(TARGET_NAME) \
                -lg7221codec-$(TARGET_NAME) \
                -lportaudio-$(TARGET_NAME)  \
                -lpj-$(TARGET_NAME) \
                -lm \
                -lpthread  \
                -lcrypto \
                -lssl \
                $(LDLIBS_$(shell uname -s)) \
                #

-include Makefile.sipsettings
sip : DEFINE += SIP_DOMAIN='"$(SIP_DOMAIN)"' \
                SIP_USER='"$(SIP_USER)"' \
                SIP_PASSWD='"$(SIP_PASSWD)"' \
                STUN_SERVER='"$(STUN_SERVER)"' \
                #

clean:
	rm -f *.o fft gen g711 sip pjtarget

