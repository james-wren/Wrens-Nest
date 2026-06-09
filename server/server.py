import http.server
import ssl
import json

class serverHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/register":
            with open("clients.json", "r") as file:
                data = json.load(file)
            
            uid = len(data)

            content = {
                "uid" : uid
            }

            print(content)
            print(uid)

            data[str(uid)] = {}

            with open("clients.json", "w") as file:
                json.dump(data, file, indent=4)

            self.send_response(200)

            self.send_header("Content-Type", "application/json")
            self.end_headers()

            self.wfile.write(json.dumps(content).encode("utf-8"))
        else:
            self.send_response_only(404)

server_add = ("localhost", 1690)

httpd = http.server.HTTPServer(server_add, serverHandler)

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)

context.load_cert_chain(certfile="cert.pem", keyfile="key.pem")

httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print("Serving")
try:
    httpd.serve_forever()
except KeyboardInterrupt:
    print("Stopped")

