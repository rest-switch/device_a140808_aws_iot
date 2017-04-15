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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>  // PATH_MAX
#include <errno.h>   // errno
// ip addr includes
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>

#include "log.h"
#include "error.h"
#include "util.h"
#include "config.h"

char thing_name[THING_NAME_SIZE+1] = { 0 };

char host_name[_POSIX_HOST_NAME_MAX+1] = { 0 };
uint16_t host_port = HOST_DEFAULT_PORT;

char iot_root_ca_path[_POSIX_PATH_MAX+1] = { 0 };
char iot_cert_path[_POSIX_PATH_MAX+1] = { 0 };
char iot_private_key_path[_POSIX_PATH_MAX+1] = { 0 };

// a140808/ak1w3b7g4
#define MQTT_TOPIC_PREFIX "a140808/"
char mqtt_subscribe_topic[sizeof(MQTT_TOPIC_PREFIX) + THING_NAME_SIZE];  // sizeof accounts for the term null


////////////////////////////////////////
int load_thing_name(char *buf, const size_t buflen)
{
    if(buflen < (THING_NAME_SIZE+1)) {
        log_error("insufficient buffer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    FILE *pfd = fopen(THING_NAME_FILEPATH, "r");
    if(NULL == pfd) {
        log_error("failed to open file " THING_NAME_FILEPATH " for read");
        return(ERROR_FILE_NOT_FOUND);
    }

    if(fseek(pfd, THING_NAME_OFFSET, SEEK_SET) < 0) {
        log_error("fseek failed, err: [%s]", strerror(errno));
        fclose(pfd);
        return(ERROR_FILE_SEEK);
    }

    const size_t b = fread(buf, 1, THING_NAME_SIZE, pfd);
    fclose(pfd);
    if(THING_NAME_SIZE != b) {
        log_error("fread failed (bytes read: %zu)", b);
        return(ERROR_FILE_READ);
    }
    buf[THING_NAME_SIZE] = '\0';
    log_info("thing name: %s", buf);

    return(SUCCESS);
}

////////////////////////////////////////
const char* get_thing_name(void)
{
    if(is_str_empty(thing_name)) {
        // load thing name
        int rc = load_thing_name(thing_name, sizeof(thing_name));
        if(SUCCESS != rc) {
            thing_name[0] = '\0';
            log_error("unable to read thing name - rc: %d", rc);
            return(NULL);
        }
    }
    return(thing_name);
}


////////////////////////////////////////
const char* get_host_name(void)
{
    return(host_name);
}

////////////////////////////////////////
int set_host_name(const char *buf)
{
    if(is_str_empty(buf)) {
        host_name[0] = '\0';
        return(SUCCESS);
    }

    if(strlen(buf) >= sizeof(host_name)) {
        log_error("insufficient buffer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    strncpy(host_name, buf, sizeof(host_name));

    log_info("host name: %s", host_name);

    return(SUCCESS);
}


////////////////////////////////////////
uint16_t get_host_port(void)
{
    return(host_port);
}

////////////////////////////////////////
int set_host_port(const char *buf)
{
    if(is_str_empty(buf)) {
        host_port = 0;
        return(SUCCESS);
    }

    host_port = (atoi(buf) & 0xffff);

    log_info("host port: %" PRIu16, host_port);

    return(SUCCESS);
}


////////////////////////////////////////
const char* get_iot_root_ca_path(void)
{
    return(iot_root_ca_path);
}

////////////////////////////////////////
int set_iot_root_ca_path(const char *buf)
{
    if(is_str_empty(buf)) {
        iot_root_ca_path[0] = '\0';
        return(SUCCESS);
    }

    if(strlen(buf) >= sizeof(iot_root_ca_path)) {
        log_error("insufficient buffer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    strncpy(iot_root_ca_path, buf, sizeof(iot_root_ca_path));

    log_info("iot root ca path: %s", iot_root_ca_path);

    return(SUCCESS);
}


////////////////////////////////////////
const char* get_iot_cert_path(void)
{
    return(iot_cert_path);
}

////////////////////////////////////////
int set_iot_cert_path(const char *buf)
{
    if(is_str_empty(buf)) {
        iot_cert_path[0] = '\0';
        return(SUCCESS);
    }

    if(strlen(buf) >= sizeof(iot_cert_path)) {
        log_error("insufficient buffer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    strncpy(iot_cert_path, buf, sizeof(iot_cert_path));

    log_info("iot cert path: %s", iot_cert_path);

    return(SUCCESS);
}


////////////////////////////////////////
const char* get_iot_private_key_path(void)
{
    return(iot_private_key_path);
}

////////////////////////////////////////
int set_iot_private_key_path(const char *buf)
{
    if(is_str_empty(buf)) {
        iot_private_key_path[0] = '\0';
        return(SUCCESS);
    }

    if(strlen(buf) >= sizeof(iot_private_key_path)) {
        log_error("insufficient buffer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    strncpy(iot_private_key_path, buf, sizeof(iot_private_key_path));

    log_info("iot private key path: %s", iot_private_key_path);

    return(SUCCESS);
}


////////////////////////////////////////
const char* get_mqtt_topic(void)
{
    int topic_len = snprintf(mqtt_subscribe_topic, sizeof(mqtt_subscribe_topic), MQTT_TOPIC_PREFIX "%s", get_thing_name());
    if(topic_len < 1) {
        log_error("error composing topic string");
        mqtt_subscribe_topic[0] = '\0';
        return(NULL);
    }
    return(mqtt_subscribe_topic);
}


////////////////////////////////////////
int get_ipv4_addresses(const char *delim, char *buf, size_t buflen)
{
    memset(buf, 0, buflen);

    struct ifaddrs* paddrs;
    if(getifaddrs(&paddrs) < 0) {
        log_error("get_ipv4_addresses:getifaddrs, err: [%s]", strerror(errno));
        return(FAILURE);
    }

    size_t out_len = 0;
    const size_t delim_len = strlen(delim);
    for(struct ifaddrs* pifa = paddrs; pifa != NULL; pifa = pifa->ifa_next)
    {
        if((NULL == pifa->ifa_addr) || (AF_INET != pifa->ifa_addr->sa_family)) {
            continue; // skip non ipv4 addresses
        }

        if(IFF_LOOPBACK == (pifa->ifa_flags & IFF_LOOPBACK)) {
            continue; // skip loopback interfaces
        }

        if((IFF_UP != (pifa->ifa_flags & IFF_UP)) || (IFF_RUNNING != (pifa->ifa_flags & IFF_RUNNING))) {
            continue; // skip interfaces that are not up and running
        }

        struct sockaddr_in *paddr = (struct sockaddr_in*)pifa->ifa_addr;
        // struct sockaddr_in *pmask = (struct sockaddr_in*)pifa->ifa_netmask;

        const char *ip = inet_ntoa(paddr->sin_addr);
        const size_t ip_len = strlen(ip);
        log_info("found ip address: %s", ip);
        if((out_len + delim_len + ip_len + 1) > buflen) {
            log_warn("buffer too small, skipping ip address: %s", ip);
            break; // cant fit any more addresses on
        }

        if(out_len > 0) {
            // add in a delim
            strcat(buf, delim);
            out_len += delim_len;
        }

        strcat(buf, ip);
        out_len += ip_len;
    }

    freeifaddrs(paddrs);

    // 1.1.1.1
    return((out_len < 7) ? FAILURE : SUCCESS);
}
