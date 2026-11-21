#!/usr/bin/env python3
# Copyright (c) 2024 Jakub Zimnol
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

import socket
import struct
import sys
import time
from argparse import ArgumentParser
from pathlib import Path

BINARY_CHUNK_SIZE = 1024

DEFAULT_PORT = 3490
DEFAULT_BINARY_PATH = './build/example_app/example_app_fota_image_encrypted.bin'


def _main():
    parser = ArgumentParser(
        description='Run TCP server, wait for connection and send new firmware file to connected device')
    parser.add_argument('-b',
                        '--binary-file',
                        help='Path to the firmware file to be sent (*.bin)',
                        default=DEFAULT_BINARY_PATH)

    parser.add_argument('-p', '--port', type=int,
                        help='TCP server port', default=DEFAULT_PORT)
    parser.add_argument('--chunk-delay', type=float, default=0,
                        help='Seconds to wait before sending each test chunk')

    args = parser.parse_args()

    print(f"Using binary path: {args.binary_file}")
    print(f"Using port: {args.port}")

    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sockfd.bind(('', args.port))
    sockfd.listen(10)

    print('Waiting for connections...')

    try:
        new_fd, addr = sockfd.accept()
        sockfd.close()
        print(f"Got connection from {addr}")

        user_input = " "
        while user_input:
            user_input = input("Press [Enter] again to update firmware")

        image = Path(args.binary_file).read_bytes()
        if len(image) < 512 or len(image) % 256:
            raise ValueError("Expected a page-aligned FOTA image with a hash trailer")

        total = len(image)
        next_progress = 10
        print(f"Sending {total} bytes (0%)", flush=True)
        with new_fd:
            new_fd.sendall(struct.pack("!I", total))
            for offset in range(0, total, BINARY_CHUNK_SIZE):
                chunk = image[offset:offset + BINARY_CHUNK_SIZE]
                time.sleep(args.chunk_delay)
                new_fd.sendall(chunk)
                sent = offset + len(chunk)
                percent = sent * 100 // total
                if percent >= next_progress:
                    print(f"Sent {sent}/{total} bytes ({percent}%)", flush=True)
                    next_progress = (percent // 10 + 1) * 10
            new_fd.shutdown(socket.SHUT_WR)
        print("Transfer sent. Check the Pico's serial monitor for verification and reboot.")

    except OSError as e:
        print(f"Update connection error: {e}")
        sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    _main()
