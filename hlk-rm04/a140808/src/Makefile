#
# Copyright 2015-2017 The REST Switch Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, Licensor provides the Work (and each Contributor provides its 
# Contributions) on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, 
# without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR 
# PURPOSE. You are solely responsible for determining the appropriateness of using or redistributing the Work and assume any 
# risks associated with Your exercise of permissions under this License.
#
# Author: John Clark (johnc@restswitch.com)
#

TARGET := a140808

.PHONY: all clean
all: $(TARGET)

SDK_DIR := external/aws-iot-sdk

INCLUDE_DIRS += -I.
INCLUDE_DIRS += -I$(SDK_DIR)/include
INCLUDE_DIRS += -I$(SDK_DIR)/external_libs/jsmn
INCLUDE_DIRS += -I$(SDK_DIR)/platform/linux/common
INCLUDE_DIRS += -I$(SDK_DIR)/platform/linux/mbedtls

SRC_FILES += main.c
SRC_FILES += config.c
SRC_FILES += msg_proc.c
SRC_FILES += serial.c
SRC_FILES += aws_iot_shadow.c
SRC_FILES += $(wildcard $(SDK_DIR)/src/*.c)
SRC_FILES += $(wildcard $(SDK_DIR)/external_libs/jsmn/*.c)
SRC_FILES += $(wildcard $(SDK_DIR)/platform/linux/common/*.c)
SRC_FILES += $(wildcard $(SDK_DIR)/platform/linux/mbedtls/*.c)

OBJ_FILES := $(SRC_FILES:.c=.o)

# logging control
#LOG_FLAGS += -DENABLE_IOT_DEBUG -g
LOG_FLAGS += -DENABLE_IOT_DEBUG
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(LOG_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)
