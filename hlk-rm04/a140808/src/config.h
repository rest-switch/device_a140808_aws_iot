//
// Copyright 2015-2017 The REST Switch Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, Licensor provides the Work (and each Contributor provides its 
// Contributions) on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, 
// without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR 
// PURPOSE. You are solely responsible for determining the appropriateness of using or redistributing the Work and assume any 
// risks associated with Your exercise of permissions under this License.
//
// Author: John Clark (johnc@restswitch.com)
//

#ifndef __config_h__
#define __config_h__

#include <stdint.h>
#include <stddef.h>


#define APP_NAME                "a140808"
#define PID_FILEPATH            "/var/run/" APP_NAME ".pid"

#define AWS_IOT_CFG_PATH        "/etc/config/aws-iot/"
#define HOST_CFG_FILEPATH       AWS_IOT_CFG_PATH "host.cfg"
#define HOST_DEFAULT_PORT       8883

#define ROOT_CA_FILEPATH        AWS_IOT_CFG_PATH "certs/root_ca.pem"
#define CERTIFICATE_FILEPATH    AWS_IOT_CFG_PATH "certs/cert.pem"
#define PRIVATE_KEY_FILEPATH    AWS_IOT_CFG_PATH "certs/private.pem"

#define SERIAL_PORT             "/dev/ttyS1"
#define SERIAL_BAUD             57600
#define SERIAL_USE_E71          true

#define THING_NAME_OFFSET       0x400
#define THING_NAME_FILEPATH     "/dev/mtd2"
#define THING_NAME_SIZE         9  // ak1w3b7g4


const char* get_thing_name(void);
const char* get_host_name(void);
uint16_t get_host_port(void);
const char* get_mqtt_topic(void);
int get_ipv4_addresses(const char *delim, char *buf, size_t buflen);


#endif // __config_h__
