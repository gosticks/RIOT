# name of your application
include ../Makefile.tests_common

BOARD_INSUFFICIENT_MEMORY := arduino-duemilanove arduino-leonardo \
                             arduino-mega2560 arduino-nano arduino-uno chronos \
                             hifive1 i-nucleo-lrwan1 msb-430 msb-430h \
                             nucleo-f030r8 nucleo-f031k6 nucleo-f042k6 \
                             nucleo-f070rb nucleo-f070rb nucleo-f072rb \
                             nucleo-f303k8 nucleo-f334r8 nucleo-l031k6 \
                             nucleo-l053r8 saml10-xpro saml11-xpro \
                             stm32f0discovery stm32l0538-disco telosb \
                             waspmote-pro wsn430-v1_3b \
                             wsn430-v1_4 z1

# use IEEE 802.15.4 as link-layer protocol
USEMODULE += netdev_ieee802154
USEMODULE += netdev_test
# 6LoWPAN and its extensions
USEMODULE += gnrc_sixlowpan_default
# UDP
USEMODULE += gnrc_udp
# Dumps packets
USEMODULE += gnrc_pktdump

TEST_ON_CI_WHITELIST += all

include $(RIOTBASE)/Makefile.include
