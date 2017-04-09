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

import time, base64, struct

JC32_REMAP = {
    'A': '0', 'J': 'j', 'S': 'u', '3': '4',
    'B': 'a', 'K': 'k', 'T': 'v', '4': '6',
    'C': 'b', 'L': 'm', 'U': 'w', '5': '7',
    'D': 'c', 'M': 'n', 'V': 'x', '6': '8',
    'E': 'd', 'N': 'p', 'W': 'y', '7': '9',
    'F': 'e', 'O': 'q', 'X': 'z',
    'G': 'f', 'P': 'r', 'Y': '1',
    'H': 'g', 'Q': 's', 'Z': '2',
    'I': 'h', 'R': 't', '2': '3',
}


def gen_serial():
    t = int(time.time() * 1000)
    enc = base64.b32encode('\x00\x00' + struct.pack(">Q", t))
    out = ''
    for c in enc:
        if out or (c != 'A'):
            out += JC32_REMAP[c]
    return(out)


if __name__ == '__main__':
    gen_serial()
