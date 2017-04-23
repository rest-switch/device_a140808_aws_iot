#!/usr/bin/env python
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

#
# https://boto3.readthedocs.io/en/latest/reference/services/iot.html
# AWS_PROFILE=dev python awsiot_policy2.py
#

from __future__ import print_function
import boto3, botocore, argparse, json


def get_certs(awsiot, chars=8):
    certs = awsiot.list_certificates()['certificates']
    result = [cert['certificateId'][:chars] for cert in certs]
    return(result)


def find_cert_arn(awsiot, cert_pre):
    certs = awsiot.list_certificates()['certificates']
    for cert in certs:
        cert_id = cert['certificateId']
        if cert_id.startswith(cert_pre):
            cert_arn = cert['certificateArn']
            #print('cert prefix: {}\n\t{}\n\t{}\n'.format(cert_pre, cert_id, cert_arn))
            return(cert_arn)
    return(None)


def create_policy(awsiot, thing_name):
    acct_id = boto3.client('sts').get_caller_identity()['Account']
    policy = {
      "Version": "2012-10-17",
      "Statement": [
        {
          "Effect": "Allow",
          "Action": [
            "iot:Publish",
            "iot:Receive"
          ],
          "Resource": [
            "arn:aws:iot:us-east-1:%s:topic/a140808/%s" % (acct_id, thing_name),
            "arn:aws:iot:us-east-1:%s:topic/$aws/things/%s/shadow/*" % (acct_id, thing_name)
          ]
        },
        {
          "Effect": "Allow",
          "Action": "iot:Subscribe",
          "Resource": [
            "arn:aws:iot:us-east-1:%s:topicfilter/a140808/%s" % (acct_id, thing_name),
            "arn:aws:iot:us-east-1:%s:topicfilter/$aws/things/%s/shadow/*" % (acct_id, thing_name)
          ]
        },
        {
          "Effect": "Allow",
          "Action": "iot:Connect",
          "Resource": "arn:aws:iot:us-east-1:%s:client/%s" % (acct_id, thing_name)
        }
      ]
    }

    policy_name = 'a140808_%s' % thing_name
    policy_doc = json.dumps(policy)
    #print(policy_doc, indent=3)

    try:
        response = awsiot.create_policy(
            policyName=policy_name,
            policyDocument=policy_doc
        )
        #print(json.dumps(response, indent=3))
    except botocore.exceptions.ClientError as ex:
        if ex.response['Error']['Code'] != 'ResourceAlreadyExistsException':
            raise

    return(policy_name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--attach', dest='cert_id',    metavar='<cert_id>',    help='attach policy for thing name to cert id (-t <thing_name>)')
    parser.add_argument('-c', '--create', dest='cert_id',    metavar='<cert_id>',    help='create and attach policy for thing name to cert id (-t <thing_name>)')
    parser.add_argument('-f', '--find',   dest='find_cert',  metavar='<cert_id>',    help='find certificate arn by id')
    parser.add_argument('-l', '--list',   dest='list_certs', metavar='chars=8',      help='list iot certificates', nargs='?',type=int,const=8)
    parser.add_argument('-n', '--new',    dest='new_cert',   action='store_true',    help='create new certificate and pub/priv keypair')
    parser.add_argument('-t', '--thing',  dest='thing_name', metavar='<thing_name>', help='thing name')
    args = parser.parse_args()

    awsiot = boto3.client('iot')

    cert_arn = None
    cert_id = None
    if args.new_cert:
        if args.cert_id:
            print('illegal option: cannot create and/or attach a policy to existing certificate while creating a new certificate')
            exit(1)

        response = awsiot.create_keys_and_certificate(setAsActive=True)
        cert_arn = response['certificateArn']
        cert_id = response['certificateId'][:8]
        cert_pem = response['certificatePem']
        pub_key = response['keyPair']['PublicKey']
        priv_key = response['keyPair']['PrivateKey']
        print('\ncert id: {}'.format(cert_id))
        print('=================')
        print('\ncert arn')
        print('========')
        print(cert_arn)
        print('\ncertificate')
        print('===========')
        print(cert_pem)
        print('\npublic key')
        print('==========')
        print(pub_key)
        print('\nprivate key')
        print('===========')
        print(priv_key)

        # attach to a policy too?
        if not args.thing_name:
            exit(0)

    if args.cert_id:
        cert_id = args.cert_id
        cert_arn = find_cert_arn(awsiot, cert_id)
        if not cert_arn:
            print('failed to find certificate: {}'.format(cert_id))
            exit(2)

    # create / attach policy to cert cert_id
    if cert_arn:
        if not args.thing_name:
            print('-t <thing_name> is required for create / attach policy operations')
            exit(3)

        policy_name = create_policy(awsiot, args.thing_name)
        awsiot.attach_principal_policy(
            policyName=policy_name,
            principal=cert_arn
        )        
        print('policy: {} attached to cert: {}'.format(policy_name, cert_id))

    # lookup cert arn by id
    if args.find_cert:
        cert_arn = find_cert_arn(awsiot, args.find_cert)
        if not cert_arn:
            print('error: cert not found')
        else:
            print(cert_arn)

    # show all cert ids
    if args.list_certs:
        certs = get_certs(awsiot, args.list_certs)
        print('\n'.join(certs))

