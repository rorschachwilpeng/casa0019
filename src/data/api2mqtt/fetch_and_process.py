import os
import json
import requests

lines = {
    "Central": {"id": "940GZZLUSTD", "directions": ["inbound"]},
    "DLR": {"id": "940GZZDLSTD", "directions": ["inbound", "outbound"]},
    "elizabeth": {"id": "910GSTFD", "directions": ["inbound"]},
    "Jubilee": {"id": "940GZZLUSTD", "directions": ["inbound"]}  # Jubilee default direction is inbound
}

api_url_template = "https://api.tfl.gov.uk/StopPoint/%s/Arrivals/"

def fetch_data_from_api(url):
    try:
        response = requests.get(url)
        if response.status_code == 200:
            return response.json()
        else:
            print(f"Error fetching data: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def filter_and_trim_data(data, target_line_name, target_direction):
    """ Extracts the first piece of data that matches the criteria """
    for item in data:
        if item.get("lineId", "").lower() == target_line_name.lower() and \
           item.get("expectedArrival") and \
           item.get("platformName") != "Platform Unknown":
            return {
                "expectedArrival": item.get("expectedArrival", ""),
                "timeToStation": item.get("timeToStation", ""),
                "direction": target_direction if target_line_name.lower() != "jubilee" else "inbound"
            }
    # Return default value if no matching data is found (applies to Jubilee)
    if target_line_name.lower() == "jubilee":
        return {
            "expectedArrival": "No data",
            "timeToStation": -1,
            "direction": "inbound"
        }
    return None

def process_all_lines_and_save():
    """ Processes all lines and consolidates data into a flat JSON file """
    output_dir = "output_json"
    os.makedirs(output_dir, exist_ok=True)

    flat_data = {}  # Stores flattened data

    for line_name, details in lines.items():
        stop_id = details["id"]
        for direction in details["directions"]:
            url = api_url_template % stop_id
            api_data = fetch_data_from_api(url)
            filtered_data = filter_and_trim_data(api_data, line_name, direction) if api_data else None
            if filtered_data:
                # Flatten data and add a prefix
                prefix = f"{line_name.lower()}_inbound_" if line_name.lower() == "jubilee" else f"{line_name.lower()}_{direction}_"
                for key, value in filtered_data.items():
                    flat_data[f"{prefix}{key}"] = value

    # Ensure Jubilee data always exists
    if "jubilee_inbound_expectedArrival" not in flat_data:
        flat_data["jubilee_inbound_expectedArrival"] = "No data"
        flat_data["jubilee_inbound_timeToStation"] = -1
        flat_data["jubilee_inbound_direction"] = "inbound"

    # Construct the path for the consolidated JSON file
    file_path = os.path.join(output_dir, "linesinfo.json")

    # Save data into a single JSON file
    with open(file_path, "w", encoding="utf-8") as json_file:
        json.dump(flat_data, json_file, ensure_ascii=False, indent=4)
