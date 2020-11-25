#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp-aether

CFLAGS += -I$(abspath ../) -DHOMEKIT_SHORT_APPLE_UUIDS

EXTRA_COMPONENT_DIRS += \
  $(abspath ../) 

include $(IDF_PATH)/make/project.mk

AE_PROJECT_PATH="$(IDF_PATH)/../esp-aether/"
AE_CONSTANTS_PATH= "${AE_PROJECT_PATH}main/constants.h"
AE_FIRMWARE_VERSION = "0.6"