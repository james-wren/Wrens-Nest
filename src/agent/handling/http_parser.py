import os
import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

def default_handler():
    with open('../stores/sys_info.json', 'r') as file:
        data = json.load(file)

    return data

def getUsageData():
    with open('../stores/sys_info.json', 'r') as file:
        data = json.load(file)

    return data["usage_data"]

ACTIONS = {
    'usage': getUsageData
}

class http_handler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_url = urlparse(self.path)
        query_params = parse_qs(parsed_url.query)

        action_param = query_params.get('action', [None][0])

        action = action_param[0] if action_param else None

        handler_function = ACTIONS.get(action, default_handler)
        response_msg = handler_function()

        self.send_response(200)

        self.send_header("Content-type", "text/plain")
        self.end_headers()

        response = str(response_msg)
        self.wfile.write(response.encode('utf-8'))

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