import os
import json
import paho.mqtt.client as mqtt

def publish_json(client, file_path, topic):
    """
    Publish a JSON file's content to a specific MQTT topic.
    
    Args:
        client: MQTT client instance.
        file_path: Path to the JSON file to be published.
        topic: MQTT topic to publish the JSON data to.
    
    Raises:
        Exception: If there is an error while reading the file or publishing the message.
    """
    try:
        # Open and read the JSON file
        with open(file_path, "r") as file:
            json_data = json.load(file)  # Parse JSON data

        # Convert JSON data to a string message
        json_message = json.dumps(json_data)

        # Publish the JSON message to the MQTT topic
        client.publish(topic, json_message)
    except Exception as e:
        # Print error if file reading or publishing fails
        print(f"Error publishing {file_path} to {topic}: {e}")

def publish_all_files(directory, topic_base, client):
    """
    Publish all JSON files in a given directory to MQTT topics.
    
    Args:
        directory: Path to the directory containing JSON files.
        topic_base: Base MQTT topic to construct the final topic for each file.
        client: MQTT client instance.
    
    Details:
        - The final MQTT topic for each file is constructed as: topic_base + file_name (without .json).
        - Only files with a .json extension are processed.
    """
    for file_name in os.listdir(directory):
        # Check if the file has a .json extension
        if file_name.endswith(".json"):
            # Construct the full file path
            file_path = os.path.join(directory, file_name)

            # Generate the MQTT topic by appending the file name to the base topic
            topic = topic_base + file_name.replace(".json", "").lower() + "/"

            # Publish the JSON file to the constructed topic
            publish_json(client, file_path, topic)
