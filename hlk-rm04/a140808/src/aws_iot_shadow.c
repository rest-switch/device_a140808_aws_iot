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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "aws_iot_config.h"

#include <jsmn.h>
#include <aws_iot_log.h>
#include <aws_iot_version.h>
//#include <aws_iot_mqtt_client_interface.h>
#include <aws_iot_shadow_interface.h>

#include "msg_proc.h"


//
// subscription endpoints
// ~~~~~~~~~~~~~~~~~~~~~~
//  $aws/things/<thing_name>/shadow/update/accepted
//  $aws/things/<thing_name>/shadow/update/rejected
//  $aws/things/<thing_name>/shadow/update/delta
//  $aws/things/<thing_name>/shadow/update/documents
//  $aws/things/<thing_name>/shadow/get/accepted
//  $aws/things/<thing_name>/shadow/get/rejected
//
// publish endpoint
// ~~~~~~~~~~~~~~~~
//  $aws/things/<thing_name>/shadow/update
//
// request endpoint
// ~~~~~~~~~~~~~~~~
//  $aws/things/<thing_name>/shadow/get
//
//
// device startup
// ~~~~~~~~~~~~~~
// 1) assume this initial cloud shadow state (output 1, 2, 3 are on):
//     {"i0":0,"i1":0,"i2":0,"i3":0,"i4":0,"i5":0,"i6":0,"i7":0,
//      "o0":0,"o1":1,"o2":1,"o3":1,"o4":0,"o5":0,"o6":0,"o7":0}
//
// 2) report power-on state (all off initially):
//     {"state":{"reported":{"i0":0,"i1":0,"i2":0,"i3":0,"i4":0,"i5":0,"i6":0,"i7":0,
//                           "o0":0,"o1":0,"o2":0,"o3":0,"o4":0,"o5":0,"o6":0,"o7":0}}}
//
// 3) delta is triggered by cloud:
//     received delta message: {"o1":1,"o2":1,"o3":1}
//
// 4) commit delta state then report it:
//     {"state":{"reported":{"o1":1,"o2":1,"o3":1}}}
//
// NOTE: It appears that there is no concept of a read-only shadow state.
//       Special care will need to be taken to never write a 'desired'
//       value to an input and/or a mechanism needs to exist to resolve
//       such a conflict or delta thrashing will occur.
//       - Perhaps the chaging of input values should be driven as a
//       'desired' state to the cloud?
//

const char *mqtt_thing_name = NULL;
AWS_IoT_Client mqttClient;

/*
 * @note The delta message is always sent on the "state" key in the json
 * @note Any time messages are bigger than AWS_IOT_MQTT_RX_BUF_LEN the underlying MQTT library will ignore it. The maximum size of the message that can be received is limited to the AWS_IOT_MQTT_RX_BUF_LEN
 */
bool messageArrivedOnDelta = false;
char stringToEchoDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];
jsonStruct_t deltaObject;


/**
 * @brief This function builds a full Shadow expected JSON document by putting the data in the reported section
 *
 * @param pJsonDocument Buffer to be filled up with the JSON data
 * @param maxSizeOfJsonDocument maximum size of the buffer that could be used to fill
 * @param pReceivedDeltaData This is the data that will be embedded in the reported section of the JSON document
 * @param lengthDelta Length of the data
 */
bool build_report_json(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta)
{
    if(NULL == pJsonDocument) {
        IOT_ERROR("build_report_json: json document is null");
        return false;
    }

    char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];
    if(aws_iot_fill_with_client_token(tempClientTokenBuffer, MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != SUCCESS) {
        IOT_ERROR("build_report_json: call to aws_iot_fill_with_client_token failed");
        return false;
    }

    int32_t ret = snprintf(pJsonDocument, maxSizeOfJsonDocument, "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}", lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);
    if(ret >= maxSizeOfJsonDocument || ret < 0) {
        IOT_ERROR("build_report_json: call to snprintf failed (ret: %d)", ret);
        return false;
    }

    return true;
}


////////////////////////////////////////
IoT_Error_t parse_json_delta(const char *json, uint32_t json_len,
                             uint8_t *input_vals, uint8_t *input_mask,
                             uint8_t *output_vals, uint8_t *output_mask)
{
    *input_vals = 0;
    *input_mask = 0;
    *output_vals = 0;
    *output_mask = 0;

    jsmn_parser parser;
    jsmn_init(&parser);

    jsmntok_t tokens[MAX_JSON_TOKEN_EXPECTED];

    int32_t token_count = jsmn_parse(&parser, json, json_len, tokens, sizeof(tokens) / sizeof(tokens[0]));
    if(token_count < 0) {
        IOT_ERROR("failed to parse json - rc: %d", token_count);
        return(FAILURE);
    }

    jsmntok_t *tok = tokens;
    if( (JSMN_ARRAY  != tok->type) &&
        (JSMN_OBJECT != tok->type) ) {
        IOT_ERROR("top-level json element must be an array or an object");
        return(FAILURE);
    }

    IOT_DEBUG("json doc: %.*s", json_len, json);
    IOT_DEBUG("token count: %d", token_count);

    // the array/object should be in key:val form
    // odd elements are keys, evens are values
    //
    // json doc: {"o1":0, "o3":0, "o5":1}
    // token count: 7
    // token 0) type: 1  range: 0 - 24  size: 3
    // token 1) type: 3  range: 2 - 4  size: 1
    // token 2) type: 4  range: 6 - 7  size: 0
    // token 3) type: 3  range: 10 - 12  size: 1
    // token 4) type: 4  range: 14 - 15  size: 0
    // token 5) type: 3  range: 18 - 20  size: 1
    // token 6) type: 4  range: 22 - 23  size: 0
    const int kv_count = tok->size;
    if((token_count-1) != (kv_count*2)) {
        IOT_ERROR("json array/object not in key/value format");
        return(FAILURE);
    }

    for(int i=0; i<kv_count; ++i) {
        jsmntok_t *key = ++tok;
        if(JSMN_STRING != key->type) {
            IOT_ERROR("element is not a key");
            return(FAILURE);
        }

        jsmntok_t *val = ++tok;
        if(JSMN_PRIMITIVE != val->type) {
            IOT_ERROR("value is not a primitive");
            return(FAILURE);
        }

        const int key_len = (key->end - key->start);
        if(2 != key_len) {
            IOT_ERROR("key is not two chars");
            return(FAILURE);
        }

        const int val_len = (val->end - val->start);
        if(1 != val_len) {
            IOT_ERROR("value is not one char");
            return(FAILURE);
        }

        // look for input/output
        const char op = json[key->start];
        uint8_t bit_num = (json[key->start + 1] - '0');
        uint8_t bit_val = (json[val->start]=='0' ? 0 : 1);
        switch(op) {
            case 'i':
                IOT_DEBUG("found input num: %d val: %d", bit_num, bit_val);
                *input_vals |= (bit_val << bit_num);
                *input_mask |= (1 << bit_num);
                break;
            case 'o':
                IOT_DEBUG("found output num: %d val: %d", bit_num, bit_val);
                *output_vals |= (bit_val << bit_num);
                *output_mask |= (1 << bit_num);
                break;
            default:
                IOT_WARN("skipping key [%.*s]", key_len, json+key->start);
                break;
        }
    }

    return(SUCCESS);
}


////////////////////////////////////////
void delta_callback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct)
{
    IOT_UNUSED(pJsonStruct);

    IOT_DEBUG("received delta message: %.*s", valueLength, pJsonValueBuffer);

    if(build_report_json(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
        messageArrivedOnDelta = true;
    }

    uint8_t input_vals = 0;
    uint8_t input_mask = 0;
    uint8_t output_vals = 0;
    uint8_t output_mask = 0;
    IoT_Error_t rc = parse_json_delta(pJsonValueBuffer, valueLength, &input_vals, &input_mask, &output_vals, &output_mask);
    if(SUCCESS != rc) {
        IOT_ERROR("failed to parse json document - rc: %d", rc);
        return;
    }

    bool brc = mp_dispatch_write_register(REG_OUTPUT_1, output_vals, output_mask);
    if(!brc) {
        IOT_ERROR("failed to write register: mp_dispatch_write_register");
    }
}


////////////////////////////////////////
void update_status_callback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status, const char *pReceivedJsonDocument, void *pContextData)
{
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    switch(status) {
        case SHADOW_ACK_TIMEOUT:
            IOT_INFO("status> update timeout --");
            break;
        case SHADOW_ACK_REJECTED:
            IOT_INFO("status> update rejected xx");
            break;
        case SHADOW_ACK_ACCEPTED:
            IOT_INFO("status> update accepted !!");
            break;
        default:
            break;
    }
}


////////////////////////////////////////
void subscribe_callback(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
    IOT_UNUSED(pData);
    IOT_UNUSED(pClient);
    IOT_DEBUG("\nSubscribe callback");
    IOT_DEBUG("------------------");
    IOT_DEBUG("  topic: %.*s", topicNameLen, topicName);
    IOT_DEBUG("  payload: %.*s", (int)params->payloadLen, (const char*)params->payload);

    const char *json = (const char*)params->payload;
    uint32_t json_len = (uint32_t)params->payloadLen;

    jsmn_parser parser;
    jsmn_init(&parser);

    // {"p0","p1","p2","p3","p4","p5","p6","p7"}
    jsmntok_t tokens[9];  // do not expect more than 8 values plus the surrounding structure

    int32_t token_count = jsmn_parse(&parser, json, json_len, tokens, sizeof(tokens) / sizeof(tokens[0]));
    if(token_count < 0) {
        IOT_ERROR("failed to parse json - rc: %d", token_count);
        return;
    }

    jsmntok_t *tok = tokens;
    if( (JSMN_ARRAY  != tok->type) &&
        (JSMN_OBJECT != tok->type) ) {
        IOT_ERROR("top-level json element must be an array or an object");
        return;
    }

    IOT_DEBUG("json doc: %.*s", json_len, json);
    IOT_DEBUG("token count: %d", token_count);

    // only expecting pulse messages right now
    uint8_t pulse_bits = 0;
    const int elem_count = tok->size;
    for(int i=0; i<elem_count; ++i) {
        jsmntok_t *elem = ++tok;
        if(JSMN_STRING != elem->type) {
            IOT_ERROR("element is not a string");
            return;
        }

        const int elem_len = (elem->end - elem->start);
        if(2 != elem_len) {
            IOT_ERROR("element is not two chars");
            return;
        }

        const char op = json[elem->start];
        if('p' != op) {
            IOT_ERROR("operation is not 'p'");
            return;
        }
        uint8_t bit_num = (json[elem->start + 1] - '0');
        pulse_bits |= (1 << bit_num);
    }

    // pulse_bits now contains a valid set of bits to pulse
    // send a pulse to each requested bit
    for(uint8_t bit=0; bit<8; ++bit) {
        if((pulse_bits >> bit) & 0x01) {
            // pulse this bit
            bool rc = mp_dispatch_pulse_register_bit(REG_OUTPUT_1, bit, 250);
            if(!rc) {
                IOT_ERROR("failed to pulse bit %d", bit);
            }
            else {
                IOT_DEBUG("pulse sent to bit %d", bit);
            }
        }
    }
}


////////////////////////////////////////
IoT_Error_t shadow_connect(const char *host_name, const uint16_t port, const char *thing_name, 
                           const char *root_ca_path, const char *cert_path, const char *private_key_path)
{
    IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqtt_thing_name = thing_name;

    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    IOT_DEBUG("connecting to: %s:%d", host_name, port);
    sp.pHost = (char*)host_name;
    sp.port = port;
    // certs
    IOT_DEBUG("root ca: %s", root_ca_path);
    IOT_DEBUG("certificate: %s", cert_path);
    IOT_DEBUG("private key: %s", private_key_path);
    sp.pRootCA = (char*)root_ca_path;
    sp.pClientCRT = (char*)cert_path;
    sp.pClientKey = (char*)private_key_path;
    sp.enableAutoReconnect = false;
    sp.disconnectHandler = NULL;

    IOT_INFO("shadow init");
    IoT_Error_t rc = aws_iot_shadow_init(&mqttClient, &sp);
    if(SUCCESS != rc) {
        IOT_ERROR("shadow connect error: %d", rc);
        return rc;
    }

    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    scp.pMyThingName = (char*)mqtt_thing_name;
    scp.pMqttClientId = (char*)mqtt_thing_name;
    scp.mqttClientIdLen = (uint16_t)strlen(mqtt_thing_name);

    IOT_INFO("shadow connect...");
    rc = aws_iot_shadow_connect(&mqttClient, &scp);
    if (SUCCESS != rc) {
        if(MQTT_REQUEST_TIMEOUT_ERROR == rc) {
            IOT_ERROR("aws_iot_shadow_connect error: MQTT_REQUEST_TIMEOUT_ERROR");
        } else {
            IOT_ERROR("aws_iot_shadow_connect error: %d", rc);
        }
        return rc;
    }
    IOT_INFO("***  shadow connected - thing name: %s  ***\n\n", mqtt_thing_name);

    // enable auto-reconnect
    //   min, max, and backoff are set in aws_iot_config.h
    //     AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
    //     AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
    rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
    if(SUCCESS != rc) {
        IOT_ERROR("failed to enable shadow autoreconnect - rc: %d", rc);
        return rc;
    }

    // register for delta callbacks
    IOT_DEBUG("registering for delta callbacks...");
//    jsonStruct_t deltaObject;
    deltaObject.pData = stringToEchoDelta;
    deltaObject.pKey = "state";
    deltaObject.type = SHADOW_JSON_OBJECT;
    deltaObject.cb = delta_callback;

    rc = aws_iot_shadow_register_delta(&mqttClient, &deltaObject);
    if(SUCCESS != rc) {
        IOT_ERROR("failed to register shadow delta callback - rc: %d", rc);
        return rc;
    }

    return(SUCCESS);
}


////////////////////////////////////////
IoT_Error_t shadow_disconnect(void)
{
    IOT_INFO("shadow disconnecting...");
    IoT_Error_t rc = aws_iot_shadow_disconnect(&mqttClient);
    if(SUCCESS != rc) {
        IOT_ERROR("disconnect error %d", rc);
        return rc;
    }
    IOT_INFO("shadow disconnected");

    return(SUCCESS);
}


////////////////////////////////////////
IoT_Error_t mqtt_subscribe(const char *topic)
{
    IOT_INFO("mqtt subscribe...");

    // /topics/a140808/ak1w3b7g4
    IoT_Error_t rc = aws_iot_mqtt_subscribe(&mqttClient, topic, strlen(topic), QOS0, subscribe_callback, NULL);
    if(SUCCESS != rc) {
        IOT_ERROR("error subscribing: %d ", rc);
        return rc;
    }

    return(SUCCESS);
}


////////////////////////////////////////
IoT_Error_t shadow_poll(void)
{
//    IOT_DEBUG("shadow poll...");

    IoT_Error_t rc = aws_iot_shadow_yield(&mqttClient, 200);
    if(NETWORK_ATTEMPTING_RECONNECT == rc) {
        IOT_INFO("shadow reconnecting...");
        return rc;
    }

    if(messageArrivedOnDelta) {
        IOT_INFO("----------------\nsending delta message back\n%s\n", stringToEchoDelta);
        rc = aws_iot_shadow_update(&mqttClient, mqtt_thing_name, stringToEchoDelta, update_status_callback, NULL, 2, true);
        if(SUCCESS != rc) {
            IOT_INFO("shadow update failed - rc = %d", rc);
            return rc;
        }

        messageArrivedOnDelta = false;
    }

    return(SUCCESS);
}

