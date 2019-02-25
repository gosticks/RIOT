#!/bin/sh
#
# cc3200 launchpad flash with Energia cc3200prog
#
# @author Attilio Dona' <@AttilioDona>

# ENERGIA_ROOT=$1
# UART_PORT=$2

# CC3200PROG_DIR=$ENERGIA_ROOT/hardware/tools/lm4f/bin/

# cd $CC3200PROG_DIR

/Users/wlad/Library/Energia15/packages/energia/tools/cc3200prog/1.1.4/cc3200prog /dev/tty.usbserial-cc3101B $3
# $CC3200PROG_DIR/cc3200prog $UART_PORT $3