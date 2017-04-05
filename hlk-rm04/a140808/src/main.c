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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h> // _SC_OPEN_MAX, setsid
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "aws_iot_shadow.h"

#include "log.h"
#include "error.h"
#include "util.h"
#include "config.h"
#include "msg_proc.h"


static bool s_run = false;


////////////////////////////////////////
void sig_term(int signum)
{
    log_info("received SIGTERM, exiting...");
    s_run = false;
//    signal(signum, SIG_DFL);
}

////////////////////////////////////////
void sig_hup(int signum)
{
    log_info("received SIGHUP");
}


////////////////////////////////////////
int main(int argc, char **argv)
{
    int opt;
    while(-1 != (opt = getopt(argc, argv, "h"))) {
        switch(opt) {
            case 'h':  // help
            default:
                printf("usage: " APP_NAME " [option]\n\n");
                printf("  -h        print this help\n\n");
                return('h'==opt ? EXIT_SUCCESS : EXIT_FAILURE);
        }
    }

    // signals
    signal(SIGHUP,  sig_hup);
    signal(SIGINT,  sig_term);
    signal(SIGTERM, sig_term);

    // create leases and pid files as 0644
    umask(022);

    log_info(APP_NAME " process started");

    // change the current working directory to root
    if(0 != chdir("/")) {
        log_error("could not change working directory to root: /, err: [%s]", strerror(errno));
        return(EXIT_FAILURE);
    }

    // store the pid
    // unlink first to ensure the file does not already exist
    unlink(PID_FILEPATH);
    FILE *pfd = fopen(PID_FILEPATH, "w");
    if(NULL == pfd) {
        log_error("could not open pid file: [" PID_FILEPATH "] for exclusive write, errno: [%s]", strerror(errno));
        return(EXIT_FAILURE);
    }
    fprintf(pfd, "%d\n", getpid());
    fclose(pfd);

    // app setup now complete
    // begin message processing

    char addrs[64] = { 0 };
    while(SUCCESS != get_ipv4_addresses("|", addrs, sizeof(addrs))) {
        sleep_ms(2000); // check for an ip address every 2 sec
    }
    addrs; // TODO: report addresses

    // connect shadow
    IoT_Error_t rc = shadow_connect(get_host_name(), get_host_port(), get_thing_name(),
                                    ROOT_CA_FILEPATH, CERTIFICATE_FILEPATH, PRIVATE_KEY_FILEPATH);
    if(SUCCESS != rc) {
        log_error("shadow connect error: %d", rc);
        unlink(PID_FILEPATH);
        return(EXIT_FAILURE);
    }
    log_info("shadow connected");

    // subscribe events
    rc = mqtt_subscribe(get_mqtt_topic());
    if(SUCCESS != rc) {
        log_error("mqtt subscribe error: %d", rc);
        unlink(PID_FILEPATH);
        return(EXIT_FAILURE);
    }
    log_info("subscribed to thing topic: %s", get_mqtt_topic());

    // init the serial message processor
    if(!mp_init(SERIAL_PORT, SERIAL_BAUD, SERIAL_USE_E71))
    {
        log_error("failed to open port: [%s]  baud: [%d]  parity: [%s]", SERIAL_PORT, SERIAL_BAUD, (SERIAL_USE_E71 ? "E71" : "N81"));
        shadow_disconnect();
        unlink(PID_FILEPATH);
        return(EXIT_FAILURE);
    }

    // main loop
    s_run = true;
    while(s_run && (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {
        rc = shadow_poll();
    }

    if(SUCCESS != rc) {
        log_error("shadow poll error, exiting...: %d", rc);
    }

    // cleanup
    log_info(APP_NAME " process closing");

    mp_close();

    rc = shadow_disconnect();
    unlink(PID_FILEPATH);
    if(SUCCESS != rc) {
        log_error("shadow disconnect error: %d", rc);
        return(EXIT_FAILURE);
    }

    log_info(APP_NAME " process closed");

    return(EXIT_SUCCESS);
}



//
// msg_proc.h callback impl
//

////////////////////////////////////////
void mp_on_pong(const uint8_t param1, const uint8_t param2, const uint8_t param3)
{
    log_debug("mp_on_pong");
}

////////////////////////////////////////
void mp_on_read_register(const uint8_t registerAddress)
{
    log_debug("mp_on_read_register");
}

////////////////////////////////////////
void mp_on_write_register(const uint8_t registerAddress, const uint8_t value, const uint8_t mask)
{
    log_debug("mp_on_write_register");
}

////////////////////////////////////////
void mp_on_write_register_bit(const uint8_t registerAddress, const uint8_t bit, const bool state)
{
    log_debug("mp_on_write_register_bit");
}

////////////////////////////////////////
void mp_on_pulse_register_bit(const uint8_t registerAddress, const uint8_t bit, const uint8_t durationMs)
{
    log_debug("mp_on_pulse_register_bit");
}

////////////////////////////////////////
void mp_on_subscribe_register(const uint8_t registerAddress, const uint8_t value, const bool cancel)
{
    log_debug("mp_on_subscribe_register");
}

