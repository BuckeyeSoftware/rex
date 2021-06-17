#!/bin/env python3

import sys
import socketserver
import ssl
from http.server import SimpleHTTPRequestHandler

PORT = 6931

class WasmHandler(SimpleHTTPRequestHandler):
  def end_headers(self):
    self.send_header("Cross-Origin-Opener-Policy", "same-origin")
    self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
    self.send_header("Cache-Control", "no-cache")
    SimpleHTTPRequestHandler.end_headers(self)

if sys.version_info < (3, 7, 5):
  WasmHandler.extensions_map['.wasm'] = 'application/wasm'

if __name__ == '__main__':
  with socketserver.TCPServer(("", PORT), WasmHandler) as httpd:
    httpd.socket = ssl.wrap_socket (httpd.socket, certfile='./server.pem', server_side=True)
    httpd.allow_reuse_address = True
    print("Listening on port {}. Press Ctrl+C to stop.".format(PORT))
    httpd.serve_forever()