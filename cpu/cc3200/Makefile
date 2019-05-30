# Define the module that is built:
MODULE = cpu

# Add a list of subdirectories, that should also be built:
DIRS += driverlib periph $(RIOTCPU)/cortexm_common

CFLAGS += -DTARGET_IS_CC3200
CFLAGS += -Wno-sign-compare -Wno-misleading-indentation -Wno-strict-prototypes -Wno-unused-parameter -Wno-old-style-definition -Wno-parentheses  -Wno-unused-variable


include $(RIOTBASE)/Makefile.base
