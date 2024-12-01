import os
import json
import numpy as np
from datetime import datetime
import requests

# Define the transportation lines and their corresponding StopPoint codes
lines = {
    "Central": "940GZZLUSTD",
    "DLR": "940GZZDLSTD",
    "elizabeth": "910GSTFD",
    "Jubilee": "940GZZLUSTD"
}

# Define the API URL template for fetching arrivals data
api_url_template = "https://api.tfl.gov.uk/StopPoint/%/Arrivals/"

def fetch_data_from_api(url):
    """
    Fetch data from the API for a given URL.
    Returns the JSON response if successful, otherwise None.
    """
    try:
        response = requests.get(url)
        if response.status_code == 200:
            return response.json()  # Successfully fetched data
        else:
            print(f"Error fetching data: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def filter_and_trim_data(data, target_line_name):
    """
    Filter and retain only relevant fields for the specified line.
    Returns a list of dictionaries with selected fields.
    """
    return [
        {
            "lineName": item.get("lineName", ""),
            "expectedArrival": item.get("expectedArrival", ""),
            "timeToStation": item.get("timeToStation", ""),
            "platformName": item.get("platformName", ""),
            "direction": item.get("direction", "")
        }
        for item in data if item.get("lineId", "").lower() == target_line_name.lower()
    ]

def filter_invalid_data(data):
    """
    Filter out records with missing expectedArrival or 'Platform Unknown'.
    """
    return [
        item for item in data
        if item.get("expectedArrival") and item.get("platformName") != "Platform Unknown"
    ]

def detect_and_handle_outliers(time_intervals, max_interval=30):
    """
    Detect and remove outliers from the time intervals.
    Keeps only intervals less than or equal to the max_interval.
    """
    return [t for t in time_intervals if t <= max_interval]

def calculate_overall_service_level(service_interval_std, waiting_times, interval_min, interval_max, wait_min, wait_max):
    """
    Calculate the overall service level based on:
    - Service interval variability (standard deviation)
    - Passenger waiting time
    Returns a normalized score between 0 and 1.
    """
    # Normalize service interval variability
    if interval_max > interval_min:
        service_interval_score = 1 - (service_interval_std - interval_min) / (interval_max - interval_min)
        service_interval_score = np.clip(service_interval_score, 0, 1)
    else:
        service_interval_score = 0.0

    # Normalize passenger waiting time
    average_wait_time = np.mean(waiting_times) if waiting_times else 0
    if wait_max > wait_min:
        waiting_time_score = 1 - (average_wait_time - wait_min) / (wait_max - wait_min)
        waiting_time_score = np.clip(waiting_time_score, 0, 1)
    else:
        waiting_time_score = 0.0

    # Combine the two metrics to calculate the overall service level
    return 0.5 * service_interval_score + 0.5 * waiting_time_score

def process_data_and_save(line_name, data):
    """
    Process data for a specific line:
    - Filter invalid data
    - Group data by direction
    - Calculate service intervals and overall service level
    - Save processed data to JSON files
    """
    if not data:
        print(f"No data for {line_name}")
        return []

    # Filter out invalid data entries
    valid_data = filter_invalid_data(data)

    # Group data by direction (e.g., inbound/outbound)
    directions = {}
    for item in valid_data:
        direction = item.get("direction", "unknown").lower()
        if direction not in directions:
            directions[direction] = []
        directions[direction].append(item)

    processed_all_data = []
    output_dir = "output_json"
    os.makedirs(output_dir, exist_ok=True)  # Ensure the output directory exists

    for direction, items in directions.items():
        # Skip processing if there are fewer than 2 records
        if len(items) < 2:
            continue

        # Parse expected arrival times into datetime objects
        expected_arrivals = [datetime.strptime(item["expectedArrival"], "%Y-%m-%dT%H:%M:%SZ") for item in items]

        # Calculate time intervals between consecutive arrivals
        time_intervals = [(expected_arrivals[i] - expected_arrivals[i-1]).total_seconds() / 60 for i in range(1, len(expected_arrivals))]
        time_intervals = detect_and_handle_outliers(time_intervals)  # Remove outliers

        if not time_intervals:
            continue  # Skip if no valid time intervals remain

        # Convert timeToStation (in seconds) to minutes
        waiting_times = [item["timeToStation"] / 60 for item in items if item.get("timeToStation")]

        # Calculate the standard deviation of service intervals
        service_interval_std = np.std(time_intervals) if time_intervals else 0

        # Compute the overall service level
        overall_service_level = calculate_overall_service_level(
            service_interval_std,
            waiting_times,
            interval_min=2, interval_max=10, wait_min=1, wait_max=20
        )

        # Add the calculated service level to the output data
        processed_items = [
            {
                "lineName": item["lineName"],
                "expectedArrival": item["expectedArrival"],
                "platformName": item["platformName"],
                "direction": item["direction"],
                "overall_service_level": round(overall_service_level, 2)
            }
            for item in items
        ]

        # Save the processed data to a JSON file
        file_path = os.path.join(output_dir, f"{line_name.replace(' ', '_')}_{direction}.json")
        with open(file_path, "w", encoding="utf-8") as json_file:
            json.dump(processed_items, json_file, ensure_ascii=False, indent=4)

        processed_all_data.extend(processed_items)

    return processed_all_data
