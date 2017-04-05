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

#include "config.h"
#include "error.h"
#include "log.h"

char thing_name[THING_NAME_SIZE+1] = { 0 };
char host_name[PATH_MAX+1] = { 0 };
uint16_t host_port = 0;

// a140808/ak1w3b7g4
#define MQTT_TOPIC_PREFIX "a140808/"
char mqtt_subscribe_topic[sizeof(MQTT_TOPIC_PREFIX) + THING_NAME_SIZE];  // sizeof includes a term null


////////////////////////////////////////
int load_thing_name(char *buf, const size_t buflen)
{
    if(buflen < (THING_NAME_SIZE+1)) {
        log_error("insufficient buffer");
        return(FAILURE);
    }

    FILE *pfd = fopen(THING_NAME_FILEPATH, "r");
    if(NULL == pfd) {
        log_error("failed to open file " THING_NAME_FILEPATH " for read");
        return(FAILURE);
    }

    if(fseek(pfd, THING_NAME_OFFSET, SEEK_SET) < 0) {
        log_error("fseek failed, err: [%s]", strerror(errno));
        fclose(pfd);
        return(FAILURE);
    }

    const size_t b = fread(buf, 1, THING_NAME_SIZE, pfd);
    fclose(pfd);
    if(THING_NAME_SIZE != b) {
        log_error("fread failed (bytes read: %zu)", b);
        return(FAILURE);
    }
    buf[THING_NAME_SIZE] = '\0';
    log_info("thing name: %s", buf);

    return(SUCCESS);
}

////////////////////////////////////////
const char* get_thing_name(void)
{
    if('\0' == thing_name[0]) {
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
int load_host_name(char *buf, const size_t buflen, uint16_t *port)
{
    *port = 0;
    if(buflen < 16)
    {
        log_error("insufficient buffer");
        return(FAILURE);
    }

    FILE *pfd = fopen(HOST_CFG_FILEPATH, "r");
    if(NULL == pfd)
    {
        log_error("failed to open file " HOST_CFG_FILEPATH " for read");
        return(FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    const ssize_t rlen = getline(&line, &len, pfd);
    fclose(pfd);
    if(rlen < 0)
    {
        log_error("fread failed");
        return(FAILURE);
    }

    if(0 == rlen)
    {
        if(line) free(line);
        buf[0] = '\0';
        return(SUCCESS);
    }

    if(rlen > buflen)
    {
        if(line) free(line);
        log_error("insufficient buffer");
        return(FAILURE);
    }

    for(size_t i=0, imax=(rlen-1); i<imax; ++i)
    {
        const char c = line[i];
        if(':' == c)
        {
            buf[i] = '\0';
            *port = (uint16_t)(((uint32_t)atoi(line + i + 1)) & 0xffff);
            break;
        }
        buf[i] = c;
    }
    buf[rlen-1] = '\0';

    if(line) free(line);

    if(0 == *port) *port = HOST_DEFAULT_PORT;

    log_info("host name: %s:%d", buf, *port);

    return(SUCCESS);
}

////////////////////////////////////////
const char* get_host_name(void)
{
    if('\0' == host_name[0]) {
        // load host name
        int rc = load_host_name(host_name, sizeof(host_name), &host_port);
        if(SUCCESS != rc) {
            host_name[0] = '\0';
            host_port = 0;
            log_error("unable to load host_name - rc: %d", rc);
            return(NULL);
        }
    }
    return(host_name);
}

////////////////////////////////////////
uint16_t get_host_port(void)
{
    if(0 == host_port) {
        // load host name
        int rc = load_host_name(host_name, sizeof(host_name), &host_port);
        if(SUCCESS != rc) {
            host_name[0] = '\0';
            host_port = 0;
            log_error("unable to load host_port - rc: %d", rc);
            return(0);
        }
    }
    return(host_port);
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
