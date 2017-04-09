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
import os, re, sys
import socket, SocketServer

MYDIR = os.path.dirname(os.path.abspath(__file__))
IN_FILEPATH = os.path.realpath(os.path.join(MYDIR, '../../bin/lede-17.01-ramips-rt305x-hlk-rm04-squashfs-sysupgrade.bin'))
OUT_FILE = 'sysupgrade.bin'
PORT = 8080


class HttpRequestHandler(SocketServer.BaseRequestHandler):
    def setup(self):
        print('client connect: \033[32m{}\033[0m'.format(self.request.getpeername()[0]));

    def handle(self):
        data = self.request.recv(1024)
        match = re.search('^GET /%s HTTP' % OUT_FILE, data)
        if(match == None):
            response  = 'HTTP/1.1 404 Not Found\n'
            response += 'content-length: 14\n'
            response += 'content-type: text/plain\n'
            response += '\n'
            response += '404 Not Found\n'
            self.request.send(response)
            return

        filename = os.path.basename(IN_FILEPATH)
        print('streaming file: \033[32m{}\033[0m to client: \033[32m{}\033[0m'.format(filename, self.request.getpeername()[0]))
        response  = 'HTTP/1.1 200 OK\n'
        response += 'content-disposition: attachment; filename=%s\n' % OUT_FILE
        response += 'content-length: %d\n' % os.path.getsize(IN_FILEPATH)
        response += 'content-type: application/octet-stream\n'
        response += '\n'
        self.request.send(response)
        # body
        sys.stdout.write('progress [')
        with open(IN_FILEPATH, 'rb') as f:
            while True:
                chunk = f.read(32768)
                if chunk:
                    sys.stdout.write('.')
                    self.request.send(chunk)
                else:
                    sys.stdout.write(']\n')
                    break

    def finish(self):
        print('client disconnect: \033[32m{}\033[0m'.format(self.request.getpeername()[0]));
        return SocketServer.BaseRequestHandler.finish(self)


def main():
    if not os.path.exists(IN_FILEPATH):
        print('error: file does not exist: {}'.format(IN_FILEPATH))
        return(1)

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        myip = s.getsockname()[0]
    except:
        print('error: failed to get local ip address')
        return(2)

    httpd = SocketServer.TCPServer(('', PORT), HttpRequestHandler)

    print('\nwaiting for connection...')
    print('  address: {}'.format(myip))
    print('  port: {}\n'.format(PORT))

    print('to upgrade the a140808 device, run the following command:')
    print('  \033[33mecho 3 > /proc/sys/vm/drop_caches && /sbin/sysupgrade -v http://{}{}/{}\033[0m\n'.format(myip, '' if(PORT==80) else ':'+str(PORT), OUT_FILE));

    print('to factory reset the a140808 device, run the following command:')
    print('  mtd -r erase rootfs_data\n')

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print('\nexiting...')
        pass
    finally:
        httpd.server_close()

    print('\nhttp server closed\n')
    return(0)


if __name__ == '__main__':
    exit(main())
