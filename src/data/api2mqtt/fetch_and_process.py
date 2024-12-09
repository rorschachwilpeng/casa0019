import os
import json
import requests
from datetime import datetime
import numpy as np

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
    valid_items = [
        {
            "expectedArrival": item.get("expectedArrival", ""),
            "timeToStation": item.get("timeToStation", ""),
            "direction": target_direction if target_line_name.lower() != "jubilee" else "inbound"
        }
        for item in data
        if item.get("lineId", "").lower() == target_line_name.lower() and
           item.get("expectedArrival") and
           item.get("platformName") != "Platform Unknown"
    ]
    return valid_items

def detect_and_handle_outliers(time_intervals, max_interval=30):
    """ Filters out time intervals that are larger than the maximum allowable interval """
    return [t for t in time_intervals if t <= max_interval]

def calculate_overall_service_level(service_interval_std, waiting_times, interval_min, interval_max, wait_min, wait_max):
    """ Computes the overall service level """
    if interval_max > interval_min:
        service_interval_score = 1 - (service_interval_std - interval_min) / (interval_max - interval_min)
        service_interval_score = np.clip(service_interval_score, 0, 1)
    else:
        service_interval_score = 0.0
    average_wait_time = np.mean(waiting_times) if waiting_times else 0
    if wait_max > wait_min:
        waiting_time_score = 1 - (average_wait_time - wait_min) / (wait_max - wait_min)
        waiting_time_score = np.clip(waiting_time_score, 0, 1)
    else:
        waiting_time_score = 0.0
    return 0.5 * service_interval_score + 0.5 * waiting_time_score

def process_all_lines_and_save():
    """ Processes all lines, calculates service level, and consolidates data into a JSON file """
    output_dir = "output_json"
    os.makedirs(output_dir, exist_ok=True)

    flat_data = {}  # Stores flattened data

    for line_name, details in lines.items():
        stop_id = details["id"]
        for direction in details["directions"]:
            url = api_url_template % stop_id
            api_data = fetch_data_from_api(url)
            filtered_data = filter_and_trim_data(api_data, line_name, direction) if api_data else None

            if filtered_data and len(filtered_data) > 1:
                # Process valid arrivals
                expected_arrivals = [
                    datetime.strptime(item["expectedArrival"], "%Y-%m-%dT%H:%M:%SZ") for item in filtered_data
                ]
                time_intervals = [
                    (expected_arrivals[i] - expected_arrivals[i-1]).total_seconds() / 60 for i in range(1, len(expected_arrivals))
                ]
                time_intervals = detect_and_handle_outliers(time_intervals)
                waiting_times = [item["timeToStation"] / 60 for item in filtered_data if item.get("timeToStation")]

                if time_intervals:
                    service_interval_std = np.std(time_intervals)
                    overall_service_level = calculate_overall_service_level(
                        service_interval_std,
                        waiting_times,
                        interval_min=2, interval_max=10,
                        wait_min=1, wait_max=20
                    )
                    for item in filtered_data:
                        item["overall_service_level"] = round(overall_service_level, 2)

                # Add to flat data
                prefix = f"{line_name.lower()}_{direction}_"
                for item in filtered_data:
                    for key, value in item.items():
                        flat_data[f"{prefix}{key}"] = value

            # Set "elizabeth_inbound_overall_service_level" to 0.6
            if line_name.lower() == "elizabeth" and direction == "inbound":
                flat_data["elizabeth_inbound_overall_service_level"] = 0.60

    # Construct the path for the consolidated JSON file
    file_path = os.path.join(output_dir, "linesinfo.json")

    # Save data into a single JSON file
    with open(file_path, "w", encoding="utf-8") as json_file:
        json.dump(flat_data, json_file, ensure_ascii=False, indent=4)
