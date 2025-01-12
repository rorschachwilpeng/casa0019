import time
import threading
import os
from flask import Flask, jsonify, send_file
from fetch_and_process import process_all_lines_and_save
from publish_to_mqtt import publish_all_files
import paho.mqtt.client as mqtt
from arduino_secrets import MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_BASE, JSON_DIRECTORY

# Flask app setup
app = Flask(__name__)

# Fetch and upload frequency (in seconds)
FETCH_UPLOAD_INTERVAL = 5

# MQTT client setup
client = mqtt.Client(client_id="PythonJSONPublisher", protocol=mqtt.MQTTv311)
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT" if rc == 0 else f"Failed to connect, code {rc}")

client.on_connect = on_connect
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()

# Flask route to serve JSON data
@app.route('/get-json/<filename>', methods=['GET'])
def get_json_file(filename):
    """
    Serve JSON files from the output directory.
    Args:
        filename: Name of the JSON file to fetch (without the .json extension).
    """
    file_path = os.path.join(JSON_DIRECTORY, f"{filename}.json")
    if os.path.exists(file_path):
        return send_file(file_path, mimetype='application/json')
    else:
        return jsonify({"error": "File not found"}), 404

# Flask route to list all available JSON files
@app.route('/list-json', methods=['GET'])
def list_json_files():
    """
    List all JSON files available in the output directory.
    """
    files = [f.replace('.json', '') for f in os.listdir(JSON_DIRECTORY) if f.endswith('.json')]
    return jsonify({"files": files})

# Background task to process data and publish to MQTT
def background_task():
    while True:
        try:
            # Fetch and process data
            process_all_lines_and_save()
            # Publish all JSON files to MQTT
            publish_all_files(JSON_DIRECTORY, MQTT_TOPIC_BASE, client)
            print(f"Data fetched and published. Waiting {FETCH_UPLOAD_INTERVAL} seconds...")
        except Exception as e:
            print(f"Error in background task: {e}")
        time.sleep(FETCH_UPLOAD_INTERVAL)

# Main entry point
if __name__ == '__main__':
    # Ensure the output directory exists
    os.makedirs(JSON_DIRECTORY, exist_ok=True)

    # Start the background task in a separate thread
    mqtt_thread = threading.Thread(target=background_task)
    mqtt_thread.daemon = True  # Ensure the thread exits when the main program exits
    mqtt_thread.start()

    # Run the Flask server
    app.run(host='0.0.0.0', port=5000)
