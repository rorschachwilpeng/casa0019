using UnityEngine;
using UnityEngine.Networking;
using System.IO;
using System.Collections;

public class APIHandler : MonoBehaviour
{
    private string apiUrl = "http://10.129.111.16:5000/get-json/linesinfo"; // API URL
    private string filePath;

    public float updateInterval = 5f; // Update every 5 seconds by default
    private string cachedJson = "";   // For caching JSON data

    void Start()
    {
        // Calculate persistent path
        filePath = Path.Combine(Application.persistentDataPath, "transportData.json");

        // Call GetJsonData function every `updateInterval` seconds
        InvokeRepeating(nameof(GetJsonData), 0f, updateInterval);
    }

    void OnDestroy()
    {
        // Ensure to stop the timed invocation when the object is destroyed
        CancelInvoke(nameof(GetJsonData));
    }

    void GetJsonData()
    {
        StartCoroutine(FetchDataFromAPI());
    }

    IEnumerator FetchDataFromAPI()
    {
        UnityWebRequest request = UnityWebRequest.Get(apiUrl);

        // Wait for the request to complete
        yield return request.SendWebRequest();

        if (request.result == UnityWebRequest.Result.Success)
        {
            string jsonResponse = request.downloadHandler.text;
            Debug.Log("API Response: " + jsonResponse);

            // Check for changes before saving
            if (jsonResponse != cachedJson)
            {
                cachedJson = jsonResponse;
                SaveJsonToFile(jsonResponse);
            }
        }
        else
        {
            Debug.LogError("Error: " + request.error);
        }
    }

    void SaveJsonToFile(string jsonData)
    {
        try
        {
            // Save JSON data to persistent path
            File.WriteAllText(filePath, jsonData);
            Debug.Log("JSON data saved to " + filePath);
        }
        catch (System.Exception e)
        {
            Debug.LogError("Error saving JSON file: " + e.Message);
        }
    }
}
