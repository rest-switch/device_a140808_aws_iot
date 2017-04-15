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

#ifndef __aws_iot_shadow_h__
#define __aws_iot_shadow_h__


#include <aws_iot_error.h>
#include <aws_iot_mqtt_client.h>


IoT_Error_t shadow_connect(const char *host_name, const uint16_t port, const char *thing_name,
						   const char *root_ca_path, const char *cert_path, const char *private_key_path);
IoT_Error_t shadow_disconnect(void);
IoT_Error_t mqtt_subscribe(const char *topic);
IoT_Error_t shadow_poll(void);


#endif // __aws_iot_shadow_h__
