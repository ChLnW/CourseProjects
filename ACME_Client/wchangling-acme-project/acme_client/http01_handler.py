from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Any
import http
from threading import Event

class HTTP01Handler(BaseHTTPRequestHandler):
    TOKEN_PATH = '/.well-known/acme-challenge'

    def do_GET(self):
        if self.path == '/':
            self.rootHandle()
        elif self.path.startswith(self.TOKEN_PATH):
            self.tokenHandle()
        else:
            self.defaultHandler()

    def rootHandle(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write("Hello World!".encode("utf-8"))

    def tokenHandle(self):
        token = self.path.split('/')[-1]
        resource = self.server.getToken(token)
        # print(token, resource, type(resource))
        if resource is None:
            self.defaultHandler()
            return
        self.send_response(http.client.OK)
        self.send_header("Content-type", "application/octet-stream")
        self.end_headers()
        self.wfile.write(resource.encode())

    def defaultHandler(self):
        self.send_response(http.client.NOT_FOUND, message="Not Found")
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(b"404")


class HTTP01Server(HTTPServer):
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.tokens: dict[str: str] = {}
        super().__init__(*args, **kwargs)
    
    def setToken(self, token: str, resource: str):
        self.tokens[token] = resource
    
    def getToken(self, token: str):
        return self.tokens[token] if token in self.tokens else None
    
    def run(self):
        try:
            self.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            # Clean-up server (close socket, etc.)
            self.server_close()

class HTTPCertificateHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == '/':
            self.tokenHandle()
        else:
            self.defaultHandler()

    def tokenHandle(self):
        token = 'cert'
        resource = self.server.getToken(token)
        if resource is None:
            self.defaultHandler()
            return
        self.send_response(http.client.OK)
        self.end_headers()
        self.wfile.write(resource.encode())

    def defaultHandler(self):
        self.send_response(http.client.NOT_FOUND, message="Not Found")
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(b"404")


class TerminateServer(HTTPServer):
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.terminate = Event()
        super().__init__(*args, **kwargs)
    
    def run(self):
        try:
            self.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            # Clean-up server (close socket, etc.)
            self.server_close()

class TerminateHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == '/shutdown':
            self.tokenHandle()
        else:
            self.defaultHandler()

    def tokenHandle(self):
        self.server.terminate.set()

    def defaultHandler(self):
        self.send_response(http.client.NOT_FOUND, message="Not Found")
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(b"404")