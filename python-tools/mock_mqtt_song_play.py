import json
import paho.mqtt.client as mqtt
import time
import argparse

parser = argparse.ArgumentParser(description='mock a song playing to mqtt, without an actual player playing it')

parser.add_argument('file', type=str, help="audio file to be mocked playing")
parser.add_argument('-p, --start_pos_ms', action="store", dest="start_pos_ms", default=0, type=int, help="start position in ms (referenced to the clock on the sending machine)")
parser.add_argument('-m, --mqtt_host', action="store", dest="mqtt_host", default='localhost', type=str, help="mqtt broker host")
results = parser.parse_args()

curr_ms_since_ephoc = int(round(time.time() * 1000))
song_start_time_ms = curr_ms_since_ephoc - results.start_pos_ms

msg = {"file_id": results.file,
       "song_is_playing": True,
       "speed": 1.0,
       "start_time_millis_since_epoch": song_start_time_ms}
msg_str = json.dumps(msg)

print(msg_str)
client = mqtt.Client()
client.connect(results.mqtt_host, 1883, 60)
client.publish("current-song", msg_str, qos=1, retain=True)