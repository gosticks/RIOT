# Define the module that is built:
MODULE = cpu

# Add a list of subdirectories, that should also be built:
DIRS += $(RIOTCPU)/cortexm_common
DIRS += periph

# (file triggers compiler bug. see #5775)
SRC_NOLTO += vectors.c

# TODO: remove or split up later
DIRS += sdk

include $(RIOTBASE)/Makefile.base
