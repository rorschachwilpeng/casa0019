import time
from fetch_and_process import process_all_lines_and_save
from publish_to_mqtt import publish_all_files
import paho.mqtt.client as mqtt
from arduino_secrets import MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_BASE, JSON_DIRECTORY

# Fetch and upload frequency (in seconds)
FETCH_UPLOAD_INTERVAL = 5

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT" if rc == 0 else f"Failed to connect, code {rc}")

# Setup MQTT client
client = mqtt.Client(client_id="PythonJSONPublisher", protocol=mqtt.MQTTv311)
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
client.on_connect = on_connect
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()

while True:
    process_all_lines_and_save()
    publish_all_files(JSON_DIRECTORY, MQTT_TOPIC_BASE, client)
    print(f"Data fetched and published. Waiting {FETCH_UPLOAD_INTERVAL} seconds...")
    time.sleep(FETCH_UPLOAD_INTERVAL)
