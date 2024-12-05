import time
from fetch_and_process import fetch_data_from_api, filter_and_trim_data, process_data_and_save, lines, api_url_template
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
    for line_name, stop_point_id in lines.items():
        api_url = api_url_template.replace("%", stop_point_id)
        json_data = fetch_data_from_api(api_url)
        filtered_data = filter_and_trim_data(json_data, line_name)
        process_data_and_save(line_name, filtered_data)
    
    publish_all_files(JSON_DIRECTORY, MQTT_TOPIC_BASE, client)
    print(f"Data fetched and published. Waiting {FETCH_UPLOAD_INTERVAL} seconds...")
    time.sleep(FETCH_UPLOAD_INTERVAL)
