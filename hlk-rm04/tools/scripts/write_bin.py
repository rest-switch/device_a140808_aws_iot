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
from collections import namedtuple
from serialnum import gen_serial

MAC1A_ADDR  = 0x40004
MAC1B_ADDR  = 0x40028
MAC2_ADDR   = 0x4002e
SERIAL_ADDR = 0x40400


def mac_bin2str(mac_bin, pnct='-'):
    if not mac_bin or len(mac_bin) != 6:
        return None
    mac_hex_str = binascii.hexlify(mac_bin)
    if len(mac_hex_str) != 12:
        return None
    return pnct.join(mac_hex_str[i:i+2] for i in range(0,12,2)) if pnct else mac_hex_str

def mac_str2bin(mac_str):
    if not mac_str:
        return None
    mac_hexstr = mac_str.replace(b':', b'').replace(b'-', b'').replace(b' ', b'')
    if len(mac_hexstr) != 12:
        return None
    mac_bin = binascii.unhexlify(mac_hexstr)
    if len(mac_bin) != 6:
        return None
    return mac_bin


def read_flash_info(image_file):
    with open(image_file, 'rb') as f:
        f.seek(MAC1B_ADDR)
        mac1_bin = f.read(6)
        f.seek(MAC2_ADDR)
        mac2_bin = f.read(6)
        f.seek(SERIAL_ADDR)
        serial_str = f.read(9)

    flash_info = namedtuple('flash_info', 'mac1 mac2 serial')
    return(flash_info(mac1=mac_bin2str(mac1_bin), mac2=mac_bin2str(mac2_bin), serial=serial_str))

def write_flash_info(image_file, mac=None, serial=None):
    with open(image_file, 'r+b') as f:
        mac1_bin = mac_str2bin(mac)
        mac2_bin = None
        if mac1_bin:
            mac1_ulong = struct.unpack('>Q', '\x00\x00'+mac1_bin)[0]
            mac2_bin = mac_str2bin('%012x' % (mac1_ulong+1))
            f.seek(MAC1A_ADDR)
            f.write(mac1_bin)
            f.seek(MAC1B_ADDR)
            f.write(mac1_bin)
            f.seek(MAC2_ADDR)
            f.write(mac2_bin)
        if serial and re.match(r'^[0abcdefghjkmnpqrstuvwxyz12346789]{9}$', serial):
            f.seek(SERIAL_ADDR)
            f.write(serial)

    flash_info = namedtuple('flash_info', 'mac1 mac2 serial')
    return(flash_info(mac1=mac_bin2str(mac1_bin), mac2=mac_bin2str(mac2_bin), serial=serial))


def gen_filename(image_file):
    match = re.match(r'^(.*?)_.*$', image_file)
    if match and match.lastindex > 0:
        base = match.group(1)
    else:
        base = "a140808"

    flash_info = read_flash_info(image_file)
    mac_unformatted = flash_info.mac1.replace(b':', b'').replace(b'-', b'').replace(b' ', b'')
    return('{}_{}_{}.bin'.format(base, mac_unformatted, flash_info.serial))


def update_devid(image_file, devid=None):
    if not devid:
        devid = gen_serial()
    else:
        match = re.match(r'^[0abcdefghjkmnpqrstuvwxyz12346789]{9}$', devid)
        if not match:
            return(1)

    write_flash_info(image_file, serial=devid)
    new_image_file_name = gen_filename(image_file)

    print()
    print('  device id for image file updated: {}'.format(image_file))
    print('        device id: {}'.format(devid))
    print('    new file name: {}'.format(new_image_file_name))
    print()

    if image_file != new_image_file_name:
        os.rename(image_file, new_image_file_name)

    return(0)


def update_mac(image_file, mac=None):
    flash_info = write_flash_info(image_file, mac=mac)
    new_image_file_name = gen_filename(image_file)

    print()
    print('  mac addresses for image file updated: {}'.format(image_file))
    print('          mac 1: {}'.format(flash_info.mac1))
    print('          mac 2: {}'.format(flash_info.mac2))
    print('  new file name: {}'.format(new_image_file_name))
    print()

    if image_file != new_image_file_name:
        os.rename(image_file, new_image_file_name)

    return(0)


def report_info(image_file):
    flash_info = read_flash_info(image_file)

    print()
    print('     image file: {}'.format(image_file))
    print('  serial number: {}'.format(flash_info.serial))
    print('          mac 1: {}'.format(flash_info.mac1))
    print('          mac 2: {}'.format(flash_info.mac2))
    print()


def main():
    parser = argparse.ArgumentParser(description='Tool to write mac address and serial number to the binary image file.')
    parser.add_argument('-g', '--genserial', dest='do_gen_serial', action='store_true', help='generate a new serial number and exit')
    parser.add_argument('-r', '--report', dest='do_report_info', action='store_true', help='report device id and mac addresses then exit')
    parser.add_argument('-d', '--devid', dest='serial_num', action='store', metavar='<serialnum>', help='set device id: -d aj3cmxeu1 (omit to generate a new serial number)')
    parser.add_argument('-m', '--mac', dest='mac_addr', action='store', metavar='<address>', help='set mac address: -m aabbccddeeff')
    parser.add_argument('-t', '--target', dest='image_file', action='store', metavar='<imgfile>', help='target binary image file')
    args = parser.parse_args()

    if args.do_gen_serial:
        print()
        print('  generating new serial number:', gen_serial())
        print()
        return(0)

    if not args.image_file:
        print('***')
        print('***  error: a target binary image file is required:  --target <bin_file>')
        print('***')
        return(12)

    target_bytes = os.path.getsize(args.image_file)
    if target_bytes < 2*1024*1024:
        print('***')
        print('***  error: {} is not a valid image file'.format(args.image_file))
        print('***')
        return(14)

    if args.do_report_info:
        report_info(args.image_file)
        return(0)

    if args.serial_num:
        rc = update_devid(args.image_file, args.serial_num)
        if rc != 0:
            print('***')
            print('***  error: the device id {} is invalid'.format(args.serial_num))
            print('***         a valid device id must be 9 chars: devid=abc123xyz')
            print('***')
            return(16)

    if args.mac_addr:
        rc = update_mac(args.image_file, args.mac_addr)
        if rc != 0:
            print('***')
            print('***  error: mac address {} is invalid'.format(args.mac_addr))
            print('***         a valid mac address must be 12 hex chars: mac=aabbccddeeff')
            print('***')
            return(18)


if __name__ == '__main__':
    exit(main())

