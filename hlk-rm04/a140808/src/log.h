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

#ifndef __log_h__
#define __log_h__

#include <stdio.h>
#include <string.h>
#include <errno.h>

// log_error   - error condition
// log_warn    - warning condition
// log_info    - informational message
// log_debug   - debug message

#define log_error(format, ...) printf("ERROR: %s %d - " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define log_warn(format, ...)  printf("WARNING: %s %d - " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define log_info(format, ...)  printf(format "\n", ##__VA_ARGS__)
#ifdef ENABLE_IOT_DEBUG
#define log_debug(format, ...) printf("DEBUG: %s %d - " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define log_debug(...)
#endif


#endif // __log_h__
