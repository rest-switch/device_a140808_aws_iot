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

from __future__ import print_function

import argparse
import binascii
import os, re, sys
import time, base64, struct
from serialnum import gen_serial

MAC1A_ADDR = 0x40004
MAC1B_ADDR = 0x40028
MAC2_ADDR = 0x4002e
SERIAL_ADDR = 0x40400


def mac_bin2str(mac_bin, pnct='-'):
    if len(mac_bin) != 6:
        return None
    mac_hexstr = binascii.hexlify(mac_bin)
    if len(mac_hexstr) != 12:
        return None
    if not pnct:
        return mac_hexstr
    return pnct.join(mac_hexstr[i:i+2] for i in range(0,12,2))

def mac_str2bin(mac_str):
    mac_hexstr = mac_str.replace(b':', b'').replace(b'-', b'')
    if len(mac_hexstr) != 12:
        return None
    mac_bin = binascii.unhexlify(mac_hexstr)
    if len(mac_bin) != 6:
        return None
    return mac_bin


def update_devid(target_bin, serial_num):
    if not serial_num:
        serial_num = gen_serial()
    else:
        match = re.match(r'^[0abcdefghjkmnpqrstuvwxyz12346789]{9}$', serial_num)
        if not match:
            return(1)

    new_target_bin = None
    match = re.search(r'.*_\w{9}\.bin$', target_bin)
    if not match:
        match = re.search(r'.*\.bin$', target_bin)
        if not match:
            new_target_bin = target_bin + '_'+serial_num+'.bin'
        else:
            new_target_bin = re.sub(r'\.bin$', '_'+serial_num+'.bin', target_bin)
    else:
        new_target_bin = re.sub(r'_\w{9}\.bin$', '_'+serial_num+'.bin', target_bin)

    print()
    print('  updating device id for image file: {}'.format(target_bin))
    print('    serial number: {}'.format(serial_num))
    print('    new file name: {}'.format(new_target_bin))
    print()

    with open(target_bin, 'r+b') as f:
        f.seek(SERIAL_ADDR)
        f.write(serial_num)

    if target_bin != new_target_bin:
        os.rename(target_bin, new_target_bin)

    return(0)


def update_mac(target_bin, mac_str):
    mac_bin1 = mac_str2bin(mac_str)
    if not mac_bin1:
        return(2)

    mac_ulong1 = struct.unpack('>Q', '\x00\x00'+mac_bin1)[0]
    mac_bin2 = mac_str2bin('%012x' % (mac_ulong1+1))

    print()
    print('  updating mac addresses for image file: {}'.format(target_bin))
    print('    mac 1: {}'.format(mac_bin2str(mac_bin1)))
    print('    mac 2: {}'.format(mac_bin2str(mac_bin2)))
    print()

    with open(target_bin, 'r+b') as f:
        f.seek(MAC1A_ADDR)
        f.write(mac_bin1)

        f.seek(MAC1B_ADDR)
        f.write(mac_bin1)

        f.seek(MAC2_ADDR)
        f.write(mac_bin2)


def report_info(target_bin):
    with open(target_bin, 'rb') as f:
        f.seek(MAC1B_ADDR)
        mac1b_addr_bin = f.read(6)
        f.seek(MAC2_ADDR)
        mac2_addr_bin = f.read(6)
        f.seek(SERIAL_ADDR)
        serial_num = f.read(9)
    print()
    print('     image file: {}'.format(target_bin))
    print('  serial number: {}'.format(serial_num))
    print('          mac 1: {}'.format(mac_bin2str(mac1b_addr_bin)))
    print('          mac 2: {}'.format(mac_bin2str(mac2_addr_bin)))
    print()


def main():
    parser = argparse.ArgumentParser(description='Tool to write mac address and serial number to the binary image file.')
    parser.add_argument('-g', '--genserial', dest='do_gen_serial', action='store_true', help='generate a new serial number and exit')
    parser.add_argument('-r', '--report', dest='do_report_info', action='store_true', help='report device id and mac addresses then exit')
    parser.add_argument('-d', '--devid', dest='serial_num', action='store', metavar='<serialnum>', help='set device id: -d aj3cmxeu1 (omit to generate a new serial number)')
    parser.add_argument('-m', '--mac', dest='mac_addr', action='store', metavar='<address>', help='set mac address: -m aabbccddeeff')
    parser.add_argument('-t', '--target', dest='target_bin', action='store', metavar='<bin_file>', help='target binary image file')
    args = parser.parse_args()

    if args.do_gen_serial:
        print()
        print('  generating new serial number:', gen_serial())
        print()
        return(0)

    if not args.target_bin:
        print('***')
        print('***  error: a target binary image file is required:  --target <bin_file>')
        print('***')
        return(12)

    target_bytes = os.path.getsize(args.target_bin)
    if target_bytes < 2*1024*1024:
        print('***')
        print('***  error: {} is not a valid image file'.format(args.target_bin))
        print('***')
        return(14)

    if args.do_report_info:
        report_info(args.target_bin)
        return(0)

    if args.serial_num:
        rc = update_devid(args.target_bin, args.serial_num)
        if rc != 0:
            print('***')
            print('***  error: the device id {} is invalid'.format(args.serial_num))
            print('***         a valid device id must be 9 chars: devid=abc123xyz')
            print('***')
            return(16)

    if args.mac_addr:
        rc = update_mac(args.target_bin, args.mac_addr)
        if rc != 0:
            print('***')
            print('***  error: mac address {} is invalid'.format(args.mac_addr))
            print('***         a valid mac address must be 12 hex chars: mac=aabbccddeeff')
            print('***')
            return(18)


if __name__ == '__main__':
    exit(main())

