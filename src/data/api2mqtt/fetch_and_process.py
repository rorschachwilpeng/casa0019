import os
import json
import requests
from datetime import datetime
import numpy as np

lines = {
    "Central": {"id": "940GZZLUSTD", "directions": ["inbound","outbound"]},
    "DLR": {"id": "940GZZDLSTD", "directions": ["inbound", "outbound"]},
    "elizabeth": {"id": "910GSTFD", "directions": ["inbound","outbound"]},
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

def filter_unique_trains(data, target_direction):
    """Filters data to keep only unique trains based on 'expectedArrival' and target direction."""
    unique_trains = {}
    for item in data:
        arrival_time = item.get("expectedArrival")
        direction = item.get("direction", "").lower()
        if arrival_time and arrival_time not in unique_trains and target_direction.lower() in direction:
            unique_trains[arrival_time] = item
    return list(unique_trains.values())

def filter_json_fields(flat_data, allowed_keys):
    """
    Filters the keys in flat_data to include only those in allowed_keys.
    
    :param flat_data: The original flattened JSON dictionary.
    :param allowed_keys: A set of keys that should be retained in the output.
    :return: A filtered dictionary containing only allowed keys.
    """
    return {
        key: value for key, value in flat_data.items()
        if any(allowed_key in key for allowed_key in allowed_keys)
    }

def process_all_lines_and_save():
    """Processes all lines, calculates service level, and consolidates data into a JSON file."""
    output_dir = "output_json"
    os.makedirs(output_dir, exist_ok=True)

    flat_data = {}  # Stores flattened data

    for line_name, details in lines.items():
        stop_id = details["id"]
        for direction in details["directions"]:
            url = api_url_template % stop_id
            print(f"Fetching data for line {line_name}, direction {direction}...")
            api_data = fetch_data_from_api(url)

            # 若 API 数据为空，直接跳过
            if not api_data:
                print(f"No data received for line {line_name}, direction {direction}. Skipping update.")
                continue

            # 特殊处理 elizabeth 线路的 outbound 方向
            if line_name.lower() == "elizabeth" and direction == "outbound":
                # 直接设置 service_level 为 0.7
                flat_data[f"{line_name.lower()}_{direction}_service_level"] = 0.7
                print(f"Set {line_name} {direction} service level to 0.7")
                continue  # 跳过后续处理

            # 过滤唯一列车数据，并筛选方向
            unique_trains = filter_unique_trains(api_data, direction)

            # 若 unique_trains 为空或没有有效数据，直接跳过
            if not unique_trains:
                print(f"No valid unique trains data for line {line_name}, direction {direction}. Skipping update.")
                continue

            # 若只有一个火车数据，跳过间隔时间计算，但可以更新数据
            if len(unique_trains) <= 1:
                print(f"Only one valid train for line {line_name}, direction {direction}. Updating with limited data.")
                flat_data[f"{line_name.lower()}_{direction}_timeToStation"] = unique_trains[0].get("timeToStation", 0)
                flat_data[f"{line_name.lower()}_{direction}_direction"] = unique_trains[0].get("direction", direction)
                # 可以选择是否设置一个默认的 service_level
                flat_data[f"{line_name.lower()}_{direction}_service_level"] = 0.0
                continue

            # 处理有效的列车到站时间
            waiting_times = [
                item["timeToStation"] / 60 for item in unique_trains if item.get("timeToStation")
            ]

            time_intervals = [
                (waiting_times[i] - waiting_times[i - 1]) for i in range(1, len(waiting_times))
            ]
            time_intervals = detect_and_handle_outliers(time_intervals)

            if time_intervals:
                service_interval_std = np.std(time_intervals)
                overall_service_level = calculate_overall_service_level(
                    service_interval_std,
                    waiting_times,
                    interval_min=2, interval_max=10,
                    wait_min=1, wait_max=20
                )
                overall_service_level = round(overall_service_level, 2)
            else:
                overall_service_level = 0.0

            # 更新扁平化数据
            prefix = f"{line_name.lower()}_{direction}_"
            # 选择第一个列车的数据进行记录
            item = unique_trains[0]
            flat_data[f"{prefix}timeToStation"] = item.get("timeToStation")
            flat_data[f"{prefix}direction"] = item.get("direction")
            flat_data[f"{prefix}service_level"] = overall_service_level

    # 过滤并保存最终数据
    allowed_keys = {"timeToStation", "direction", "service_level"}
    filtered_data = filter_json_fields(flat_data, allowed_keys)
    file_path = os.path.join(output_dir, "linesinfo.json")
    with open(file_path, "w", encoding="utf-8") as json_file:
        json.dump(filtered_data, json_file, ensure_ascii=False, indent=4)
