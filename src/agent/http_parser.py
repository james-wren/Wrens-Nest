from http.server import BaseHTTPRequestHandler, HTTPServer

class http_handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)

        self.send_header("Content-type", "text/plain")
        self.end_headers()

        test_message = 'Server working\n'
        self.wfile.write(test_message.encode('utf-8'))

def run_server(port=1690):
    server_address = ('', port)
    httpd = HTTPServer(server_address, http_handler)

    print(f'Server running on port: {port}')
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print('Server stopped')

if __name__ == '__main__':
    run_server()