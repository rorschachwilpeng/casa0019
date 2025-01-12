using UnityEngine;
using TMPro;
using System.IO;
using System.Collections;
using Newtonsoft.Json.Linq;

public class APIRequest4 : MonoBehaviour
{
    public TextMeshProUGUI[] serviceLevelTexts; // Array of 8 TextMeshPro objects
    public float interval = 5f; // Update interval
    private string filePath; // File path

    private void Start()
    {
        // Set file path to persistent data path
        filePath = Path.Combine(Application.persistentDataPath, "transportData.json");
        
        // Start periodic service level updates
        StartCoroutine(UpdateServiceLevels());
    }

    private IEnumerator UpdateServiceLevels()
    {
        while (true)
        {
            // Check if file exists
            if (File.Exists(filePath))
            {
                // Read file content
                string jsonContent = File.ReadAllText(filePath);
                JObject json = JObject.Parse(jsonContent);

                // Get and populate service level data
                UpdateText(serviceLevelTexts[0], json, "central_inbound_service_level");
                UpdateText(serviceLevelTexts[1], json, "central_outbound_service_level");
                UpdateText(serviceLevelTexts[2], json, "dlr_inbound_service_level");
                UpdateText(serviceLevelTexts[3], json, "dlr_outbound_service_level");
                UpdateText(serviceLevelTexts[4], json, "elizabeth_inbound_service_level");
                UpdateText(serviceLevelTexts[5], json, "elizabeth_outbound_service_level");
                UpdateText(serviceLevelTexts[6], json, "jubilee_inbound_service_level");
                UpdateText(serviceLevelTexts[7], json, "jubilee_outbound_service_level");
            }
            else
            {
                Debug.LogError("File not found: " + filePath);
            }

            // Wait for specified interval
            yield return new WaitForSeconds(interval);
        }
    }

    private void UpdateText(TextMeshProUGUI textObject, JObject json, string key)
    {
        // Get service level value from JSON, use default value 0.5 if not found
        float serviceLevel = json[key]?.Value<float>() ?? 0.5f;
        textObject.text = serviceLevel.ToString("F2");
    }
}
