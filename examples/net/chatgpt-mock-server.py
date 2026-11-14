#!/usr/bin/env python3
import itertools
import json
import os
import re
import socket
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit

HOST = os.environ.get('HOST', 'localhost')
PORT = int(os.environ.get('PORT', '8787'))
DELAY_SECONDS = int(os.environ.get('MOCK_DELAY_MS', '25')) / 1000
EXPECTED_KEY = os.environ.get('MOCK_CHATGPT_API_KEY', '')
BODY_LIMIT = 1000000
COMPLETIONS_PATH = '/v1/chat/completions'
SERVER_NAME = 'yumi-chatgpt-mock'
FINGERPRINT = 'fp_yumi_mock'
CHUNKS = re.compile(r'\s+|\S+')
DIGITS = '0123456789abcdefghijklmnopqrstuvwxyz'
SERIALS = itertools.count(1)
SERIAL_LOCK = threading.Lock()


def say(message):
    print(message, flush=True)


def base36(number):
    text = ''
    while True:
        number, digit = divmod(number, 36)
        text = DIGITS[digit] + text
        if not number:
            return text


def request_id():
    with SERIAL_LOCK:
        serial = next(SERIALS)
    return 'chatcmpl-mock-%s-%s' % (base36(int(time.time() * 1000)), base36(serial))


def message_text(content):
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return ''.join(part['text'] for part in content if isinstance(part, dict) and isinstance(part.get('text'), str))
    return ''


def last_user_text(messages):
    for message in reversed(messages):
        if isinstance(message, dict) and message.get('role') == 'user':
            return message_text(message.get('content'))
    return ''


def token_count(text):
    return len(text.split())


def usage(messages, content):
    prompt_tokens = sum(token_count(message_text(message['content'])) for message in messages)
    completion_tokens = token_count(content)
    return {
        'prompt_tokens': prompt_tokens,
        'completion_tokens': completion_tokens,
        'total_tokens': prompt_tokens + completion_tokens,
    }


def encoded(payload):
    return json.dumps(payload, separators=(',', ':'), ensure_ascii=False)


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, format, *arguments):
        pass

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Headers', 'authorization, content-type')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Content-Length', '0')
        self.end_headers()

    def do_GET(self):
        if urlsplit(self.path).path == '/health':
            self.send_json(200, {'ok': True, 'server': SERVER_NAME})
        else:
            self.route()

    def do_POST(self):
        self.route()

    def route(self):
        path = urlsplit(self.path).path
        if path != COMPLETIONS_PATH or self.command != 'POST':
            self.close_connection = True
        if path != COMPLETIONS_PATH:
            self.send_failure(404, 'No mock route for %s %s' % (self.command, path))
            return
        if self.command != 'POST':
            self.send_failure(405, 'Only POST is supported for %s' % COMPLETIONS_PATH)
            return
        try:
            body = self.read_json()
        except ValueError as error:
            self.send_failure(400, str(error))
            return
        if not self.valid(body):
            return
        stream = body.get('stream') is True
        count = len(body['messages'])
        say('%s %s  %s, %d message%s%s' % (self.command, path, body['model'], count, '' if count == 1 else 's',
                                          ', streaming' if stream else ''))
        try:
            if stream:
                self.stream(body)
            else:
                self.complete(body)
        except (BrokenPipeError, ConnectionResetError):
            say('The client disconnected')

    def read_json(self):
        if self.headers.get('Transfer-Encoding', '').lower() == 'chunked':
            data = self.read_chunked()
        else:
            length = int(self.headers.get('Content-Length') or 0)
            if length > BODY_LIMIT:
                self.close_connection = True
                raise ValueError('Request body is too large for this mock server')
            data = self.rfile.read(length)
        try:
            return json.loads(data.decode('utf-8') or '{}')
        except ValueError as error:
            raise ValueError('Invalid JSON body: %s' % error)

    def read_chunked(self):
        data = b''
        while True:
            size = int(self.rfile.readline().split(b';')[0], 16)
            if not size:
                self.rfile.readline()
                return data
            data += self.rfile.read(size)
            self.rfile.readline()
            if len(data) > BODY_LIMIT:
                self.close_connection = True
                raise ValueError('Request body is too large for this mock server')

    def valid(self, body):
        authorization = self.headers.get('Authorization', '')
        if not authorization.startswith('Bearer '):
            self.send_failure(401, 'Missing Authorization bearer token', 'authentication_error')
            return False
        if EXPECTED_KEY and authorization != 'Bearer ' + EXPECTED_KEY:
            self.send_failure(401, 'Authorization bearer token did not match MOCK_CHATGPT_API_KEY',
                              'authentication_error')
            return False
        if 'application/json' not in self.headers.get('Content-Type', ''):
            self.send_failure(415, 'Content-Type must be application/json')
            return False
        if not isinstance(body, dict) or not isinstance(body.get('model'), str) or not body['model']:
            self.send_failure(400, 'Request body must include a string model')
            return False
        if not isinstance(body.get('messages'), list):
            self.send_failure(400, 'Request body must include a messages array')
            return False
        for index, message in enumerate(body['messages']):
            if not isinstance(message, dict) or not isinstance(message.get('role'), str):
                self.send_failure(400, 'messages[%d].role must be a string' % index)
                return False
            if not isinstance(message.get('content'), (str, list)):
                self.send_failure(400, 'messages[%d].content must be a string or content array' % index)
                return False
        return True

    def stream(self, body):
        identifier = request_id()
        content = 'Mock ChatGPT streaming OK. I received: %s' % json.dumps(last_user_text(body['messages']),
                                                                           ensure_ascii=False)
        options = body.get('stream_options')
        common = {
            'id': identifier,
            'object': 'chat.completion.chunk',
            'created': int(time.time()),
            'model': body['model'],
            'service_tier': 'default',
            'system_fingerprint': FINGERPRINT,
        }
        self.send_response(200)
        for name, value in (
            ('Content-Type', 'text/event-stream; charset=utf-8'),
            ('Cache-Control', 'no-cache, no-transform'),
            ('Connection', 'keep-alive'),
            ('X-Accel-Buffering', 'no'),
            ('X-Request-Id', identifier),
            ('OpenAI-Processing-Ms', '1'),
            ('Access-Control-Allow-Origin', '*'),
            ('Transfer-Encoding', 'chunked'),
        ):
            self.send_header(name, value)
        self.end_headers()
        deltas = [{'role': 'assistant'}] + [{'content': chunk} for chunk in CHUNKS.findall(content)] + [{}]
        for index, delta in enumerate(deltas):
            choice = {
                'index': 0,
                'delta': delta,
                'logprobs': None,
                'finish_reason': 'stop' if index == len(deltas) - 1 else None,
            }
            self.send_event(dict(common, choices=[choice], usage=None))
        if isinstance(options, dict) and options.get('include_usage') is True:
            self.send_event(dict(common, choices=[], usage=usage(body['messages'], content)))
        self.send_chunk(b'data: [DONE]\n\n')
        self.send_chunk(b'')
        say('streamed %d chunks for %s' % (len(deltas) - 2, identifier))

    def send_event(self, payload):
        self.send_chunk(('data: %s\n\n' % encoded(payload)).encode('utf-8'))
        if DELAY_SECONDS > 0:
            time.sleep(DELAY_SECONDS)

    def send_chunk(self, data):
        self.wfile.write(b'%x\r\n%s\r\n' % (len(data), data))
        self.wfile.flush()

    def complete(self, body):
        identifier = request_id()
        content = 'Mock ChatGPT non-streaming OK. I received: %s' % json.dumps(last_user_text(body['messages']),
                                                                               ensure_ascii=False)
        self.send_json(200, {
            'id': identifier,
            'object': 'chat.completion',
            'created': int(time.time()),
            'model': body['model'],
            'service_tier': 'default',
            'system_fingerprint': FINGERPRINT,
            'choices': [
                {
                    'index': 0,
                    'message': {'role': 'assistant', 'content': content},
                    'logprobs': None,
                    'finish_reason': 'stop',
                },
            ],
            'usage': usage(body['messages'], content),
        })

    def send_failure(self, status, message, kind='invalid_request_error'):
        say('%d %s' % (status, message))
        self.send_json(status, {'error': {'message': message, 'type': kind, 'param': None, 'code': None}})

    def send_json(self, status, payload):
        data = encoded(payload).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main():
    family, _, _, _, address = socket.getaddrinfo(HOST, PORT, type=socket.SOCK_STREAM)[0]
    server_class = type('Server', (ThreadingHTTPServer,), {'address_family': family})
    server = server_class(address, Handler)
    say('Mock ChatGPT server on http://%s:%d, chat completions at http://localhost:%d%s, %dms between chunks'
        % (HOST, PORT, PORT, COMPLETIONS_PATH, DELAY_SECONDS * 1000))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
