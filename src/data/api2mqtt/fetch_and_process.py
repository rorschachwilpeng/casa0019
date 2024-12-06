import os
import json
import requests

lines = {
    "Central": {"id": "940GZZLUSTD", "directions": ["inbound"]},
    "DLR": {"id": "940GZZDLSTD", "directions": ["inbound", "outbound"]},
    "elizabeth": {"id": "910GSTFD", "directions": ["inbound"]},
    "Jubilee": {"id": "940GZZLUSTD", "directions": ["inbound"]}  # Jubilee 默认方向为 inbound
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
    """ 只抓取第一个符合条件的数据 """
    for item in data:
        if item.get("lineId", "").lower() == target_line_name.lower() and \
           item.get("expectedArrival") and \
           item.get("platformName") != "Platform Unknown":
            return {
                "expectedArrival": item.get("expectedArrival", ""),
                "timeToStation": item.get("timeToStation", ""),
                "direction": target_direction if target_line_name.lower() != "jubilee" else "inbound"
            }
    # 如果没有符合条件的数据，返回默认值（适用于 Jubilee）
    if target_line_name.lower() == "jubilee":
        return {
            "expectedArrival": "No data",
            "timeToStation": -1,
            "direction": "inbound"
        }
    return None

def process_all_lines_and_save():
    """ 处理所有线路并整合到一个扁平化 JSON 文件中 """
    output_dir = "output_json"
    os.makedirs(output_dir, exist_ok=True)

    flat_data = {}  # 用于存储扁平化数据

    for line_name, details in lines.items():
        stop_id = details["id"]
        for direction in details["directions"]:
            url = api_url_template % stop_id
            api_data = fetch_data_from_api(url)
            filtered_data = filter_and_trim_data(api_data, line_name, direction) if api_data else None
            if filtered_data:
                # 将数据展平并添加前缀
                prefix = f"{line_name.lower()}_inbound_" if line_name.lower() == "jubilee" else f"{line_name.lower()}_{direction}_"
                for key, value in filtered_data.items():
                    flat_data[f"{prefix}{key}"] = value

    # 确保 Jubilee 数据始终存在
    if "jubilee_inbound_expectedArrival" not in flat_data:
        flat_data["jubilee_inbound_expectedArrival"] = "No data"
        flat_data["jubilee_inbound_timeToStation"] = -1
        flat_data["jubilee_inbound_direction"] = "inbound"

    # 构建整合后的 JSON 文件路径
    file_path = os.path.join(output_dir, "linesinfo.json")

    # 保存数据到单个 JSON 文件
    with open(file_path, "w", encoding="utf-8") as json_file:
        json.dump(flat_data, json_file, ensure_ascii=False, indent=4)