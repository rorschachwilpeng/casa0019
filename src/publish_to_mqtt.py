import os
import json
import paho.mqtt.client as mqtt

def publish_json(client, file_path, topic):
    try:
        with open(file_path, "r") as file:
            json_data = json.load(file)
        json_message = json.dumps(json_data)
        client.publish(topic, json_message)
    except Exception as e:
        print(f"Error publishing {file_path} to {topic}: {e}")

def publish_all_files(directory, topic_base, client):
    for file_name in os.listdir(directory):
        if file_name.endswith(".json"):
            file_path = os.path.join(directory, file_name)
            topic = topic_base + file_name.replace(".json", "").lower() + "/"
            publish_json(client, file_path, topic)
