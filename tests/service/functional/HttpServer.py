from http.server import HTTPServer, BaseHTTPRequestHandler

class HttpServer(HTTPServer):
    allow_reuse_address = True

    def __init__(self, handler):
        super().__init__(('', 8000), handler)


class HttpRequestHandler(BaseHTTPRequestHandler):
    pass
