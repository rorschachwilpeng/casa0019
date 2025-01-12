using UnityEngine;
using System;
using System.Collections.Generic;
using TMPro;
using Newtonsoft.Json.Linq;
using System.IO;

public class APIRequest3 : MonoBehaviour
{
    // Reference to TMP_Text components for displaying data
    public TMP_Text[] trainTexts; // 8 TMP_Text components that can be assigned in Inspector

    // JSON file path (using persistent data path)
    private string jsonFilePath;

    // Public variable to control update interval (in seconds)
    public float updateInterval = 5f; // Default value set to 5 seconds

    void Start()
    {
        // Set file path to persistent data path
        jsonFilePath = Path.Combine(Application.persistentDataPath, "transportData.json");

        // Start periodic UpdateData method calls based on updateInterval
        InvokeRepeating(nameof(UpdateData), 0f, updateInterval);
    }

    // Method to update data based on configured time interval
    void UpdateData()
    {
        // Ensure file exists
        if (File.Exists(jsonFilePath))
        {
            // Load and process JSON data from file
            string jsonContent = File.ReadAllText(jsonFilePath);
            ProcessData(jsonContent);
        }
        else
        {
            Debug.LogWarning("JSON file not found at " + jsonFilePath);
        }
    }

    void ProcessData(string jsonResponse)
    {
        try
        {
            // Parse JSON response to JObject
            JObject jsonObject = JObject.Parse(jsonResponse);

            // Prepare log content (for debugging)
            string logContent = "";

            // Store data grouped by StopPointId
            var stopPointData = new Dictionary<string, List<TrainInfo>>();

            foreach (var property in jsonObject.Properties())
            {
                string key = property.Name; // Example: "central_inbound_timeToStation"
                string[] parts = key.Split('_');
                if (parts.Length != 3)
                    continue;

                string lineName = parts[0];
                string direction = parts[1];
                string attribute = parts[2];
                var value = property.Value;

                // Initialize list for this StopPointId if first encounter
                if (!stopPointData.ContainsKey(lineName))
                {
                    stopPointData[lineName] = new List<TrainInfo>();
                }

                // Process arrival time
                if (attribute == "timeToStation")
                {
                    TrainInfo train = new TrainInfo
                    {
                        LineName = lineName,
                        Direction = direction,
                        TimeToStation = value.ToObject<int>()
                    };
                    stopPointData[lineName].Add(train);
                }
            }

            int textIndex = 0; // For tracking TMP_Text index

            // Now that we have collected all train data, format and update TMP_Text components
            foreach (var stopPoint in stopPointData)
            {
                logContent += $"StopPointId: {stopPoint.Key}\n";

                // Separate inbound and outbound trains by direction
                List<TrainInfo> inboundTrains = stopPoint.Value.FindAll(t => t.Direction == "inbound");
                List<TrainInfo> outboundTrains = stopPoint.Value.FindAll(t => t.Direction == "outbound");

                // Update inbound train information to TMP_Text
                UpdateTrainTexts(ref textIndex, inboundTrains);

                // Update outbound train information to TMP_Text
                UpdateTrainTexts(ref textIndex, outboundTrains);

                logContent += "\n"; // Add line break after each StopPointId block
            }

            // Print log content to console (for debugging)
            Debug.Log(logContent);
        }
        catch (Exception ex)
        {
            Debug.LogError($"Error processing data: {ex.Message}");
        }
    }

    void UpdateTrainTexts(ref int textIndex, List<TrainInfo> trainInfos)
    {
        for (int i = 0; i < trainInfos.Count; i++) // Iterate through train information
        {
            if (textIndex >= trainTexts.Length)
            {
                break; // Avoid accessing beyond array bounds
            }

            TrainInfo train = trainInfos[i];
            // Convert time to minutes
            int minutesToArrival = Mathf.Max(0, train.TimeToStation / 60); // Convert seconds to minutes

            // Update TMP_Text component to display arrival time
            trainTexts[textIndex].text = $"{minutesToArrival}";

            // Increment textIndex for next TMP_Text update
            textIndex++;
        }

        // Fill remaining slots with default values if not enough train data
        int fillCount = 2 - trainInfos.Count; // Expect 2 data points for each direction
        for (int i = 0; i < fillCount; i++)
        {
            if (textIndex >= trainTexts.Length)
            {
                break; // Avoid accessing beyond array bounds
            }

            trainTexts[textIndex].text = "10"; // Default value
            textIndex++;
        }
    }
}

// Class for storing train information
public class TrainInfo
{
    public string LineName { get; set; }    // Line name
    public string Direction { get; set; }    // Direction (inbound or outbound)
    public int TimeToStation { get; set; }   // Time to arrival (in seconds)
}
