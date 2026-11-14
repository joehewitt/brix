#!/usr/bin/env python3
import base64
import hashlib
import os
import socket
import struct
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = os.environ.get('HOST', 'localhost')
PORT = int(os.environ.get('PORT', '8765'))
GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'
TEXT = 0x1
CLOSE = 0x8
PING = 0x9
PONG = 0xA
FINAL = 0x80
MASKED = 0x80
SHORT_LENGTH = 126
LONG_LENGTH = 127


def say(message):
    print(message, flush=True)


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, format, *arguments):
        pass

    def do_GET(self):
        if self.headers.get('Upgrade', '').lower() != 'websocket':
            data = b'Use a WebSocket client for this endpoint.\n'
            self.send_response(426)
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        self.close_connection = True
        key = self.headers.get('Sec-WebSocket-Key')
        if not key:
            return
        accept = base64.b64encode(hashlib.sha1((key + GUID).encode('ascii')).digest()).decode('ascii')
        lines = [
            'HTTP/1.1 101 Switching Protocols',
            'Upgrade: websocket',
            'Connection: Upgrade',
            'Sec-WebSocket-Accept: ' + accept,
        ]
        protocols = [protocol.strip() for protocol in self.headers.get('Sec-WebSocket-Protocol', '').split(',')]
        if 'json' in protocols:
            lines.append('Sec-WebSocket-Protocol: json')
        self.wfile.write(('\r\n'.join(lines) + '\r\n\r\n').encode('ascii'))
        self.wfile.flush()
        say('client connected: %s' % self.path)
        try:
            self.converse()
        except (BrokenPipeError, ConnectionResetError):
            pass
        say('client disconnected')

    def converse(self):
        while True:
            frame = self.read_frame()
            if frame is None:
                return
            opcode, payload = frame
            if opcode == TEXT:
                text = payload.decode('utf-8', 'replace')
                say('received text: %s' % text)
                self.send_frame(TEXT, ('echo: ' + text).encode('utf-8'))
            elif opcode == CLOSE:
                self.send_frame(CLOSE, payload)
                return
            elif opcode == PING:
                self.send_frame(PONG, payload)

    def read_frame(self):
        head = self.read_exactly(2)
        if head is None:
            return None
        length = head[1] & 0x7F
        if length == SHORT_LENGTH:
            extended = self.read_exactly(2)
            if extended is None:
                return None
            length = struct.unpack('>H', extended)[0]
        elif length == LONG_LENGTH:
            extended = self.read_exactly(8)
            if extended is None:
                return None
            length = struct.unpack('>Q', extended)[0]
        mask = self.read_exactly(4) if head[1] & MASKED else b''
        payload = self.read_exactly(length)
        if mask is None or payload is None:
            return None
        if mask:
            payload = bytes(byte ^ mask[index % 4] for index, byte in enumerate(payload))
        return head[0] & 0x0F, payload

    def read_exactly(self, count):
        data = self.rfile.read(count)
        return data if len(data) == count else None

    def send_frame(self, opcode, payload):
        length = len(payload)
        if length < SHORT_LENGTH:
            header = struct.pack('>BB', FINAL | opcode, length)
        elif length <= 0xFFFF:
            header = struct.pack('>BBH', FINAL | opcode, SHORT_LENGTH, length)
        else:
            header = struct.pack('>BBQ', FINAL | opcode, LONG_LENGTH, length)
        self.wfile.write(header + payload)
        self.wfile.flush()


def main():
    family, _, _, _, address = socket.getaddrinfo(HOST, PORT, type=socket.SOCK_STREAM)[0]
    server_class = type('Server', (ThreadingHTTPServer,), {'address_family': family})
    server = server_class(address, Handler)
    say('websocket test server listening on ws://%s:%d/' % (HOST, PORT))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
