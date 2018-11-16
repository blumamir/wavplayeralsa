
import player_command_pb2
import zmq
import argparse

parser = argparse.ArgumentParser(description="script to send 'stop play' command to wavplayeralsa")
parser.add_argument('--player_port', type=int, default=2100, help='the port on which the player listens for commands')
parser.add_argument('--player_host', type=str, default="localhost", help='the host on which the player is running')
args = parser.parse_args()

msg = player_command_pb2.PlayerCommandMsg()
msg.req_identifier.cookie = 1
msg.req_identifier.requestor_guid = 1
msg.req_identifier.requestor_name = "send-stop-script"
msg.stop_play = True

context = zmq.Context()
print "Connecting to server..."
socket = context.socket(zmq.REQ)
socket.connect ("tcp://%s:%s" % (args.player_host, args.player_port))

print "Sending request ", str(msg), "..."
socket.send(msg.SerializeToString())
#  Get the reply.
binary_rep = socket.recv()
proto_rep = player_command_pb2.PlayerCommandReplyMsg()
proto_rep.ParseFromString(binary_rep)

print "Received reply: ", str(proto_rep)

