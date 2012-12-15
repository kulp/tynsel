ifneq ($(DEBUG),)
CFLAGS += -g -O0
CPPFLAGS += -DDEBUG
endif

CPPFLAGS += $(patsubst %,-D%,$(DEFINE))

CFLAGS += -Wall -Wextra -Wunused

fft: CPPFLAGS += -std=c99
fft: LDLIBS += -lfftw3
g711 gen fft: LDLIBS += -lsndfile

all: fft gen sip

# detect gives us the TARGET_NAME for linking
detect: LDLIBS =
detect: CPPFLAGS =
sip: | detect
sip: TARGET_NAME = $(shell ./detect)

LDLIBS_Darwin += \
               -framework CoreAudio \
               -framework CoreServices \
               -framework AudioUnit \
               -framework AudioToolbox \
			   #

sip: LDLIBS := \
               -lpjsua-$(TARGET_NAME) \
               -lpjsip-ua-$(TARGET_NAME) \
               -lpjsip-simple-$(TARGET_NAME) \
               -lpjsip-$(TARGET_NAME) \
               -lpjmedia-codec-$(TARGET_NAME) \
               -lpjmedia-videodev-$(TARGET_NAME) \
               -lpjmedia-$(TARGET_NAME) \
               -lpjmedia-audiodev-$(TARGET_NAME) \
               -lpjnath-$(TARGET_NAME) \
               -lpjlib-util-$(TARGET_NAME) \
               -lresample-$(TARGET_NAME) \
               -lmilenage-$(TARGET_NAME) \
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

DEFINE += SIP_DOMAIN='"$(SIP_DOMAIN)"' \
          SIP_USER='"$(SIP_USER)"' \
          SIP_PASSWD='"$(SIP_PASSWD)"' \
          STUN_SERVER='"$(STUN_SERVER)"' \
          #

clean:
	rm -f *.o fft gen sip detect

