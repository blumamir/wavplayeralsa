import argparse
import http.client
import json

parser = argparse.ArgumentParser(description='set new audio file for wavplayeralsa server')

parser.add_argument('--ip_address', action="store", dest="ip_address", default="127.0.0.1", type=str, help="ip or host name of the http server (player's host)")
parser.add_argument('--port', action="store", dest="port", default=8080, type=int, help="port of the http server (player's host)")
results = parser.parse_args()

json_req = {}
json_str = json.dumps(json_req)

connection = http.client.HTTPConnection(results.ip_address, results.port)
connection.request("PUT", "/api/current-song", json_str)
response = connection.getresponse()
print("Status: {} and reason: {}".format(response.status, response.reason))
print(response.read())

connection.close()

