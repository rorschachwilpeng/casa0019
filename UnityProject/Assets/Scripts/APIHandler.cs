using UnityEngine;
using UnityEngine.Networking;
using System.IO;
using System.Collections;

public class APIHandler : MonoBehaviour
{
    private string apiUrl = "http://10.129.111.16:5000/get-json/linesinfo"; // API URL
    private string filePath;

    public float updateInterval = 5f; 
    private string cachedJson = "";  

    void Start()
    {
        filePath = Path.Combine(Application.persistentDataPath, "transportData.json");

        InvokeRepeating(nameof(GetJsonData), 0f, updateInterval);
    }

    void OnDestroy()
    {
        CancelInvoke(nameof(GetJsonData));
    }

    void GetJsonData()
    {
        StartCoroutine(FetchDataFromAPI());
    }

    IEnumerator FetchDataFromAPI()
    {
        UnityWebRequest request = UnityWebRequest.Get(apiUrl);

        yield return request.SendWebRequest();

        if (request.result == UnityWebRequest.Result.Success)
        {
            string jsonResponse = request.downloadHandler.text;
            Debug.Log("API Response: " + jsonResponse);

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
            File.WriteAllText(filePath, jsonData);
            Debug.Log("JSON data saved to " + filePath);
        }
        catch (System.Exception e)
        {
            Debug.LogError("Error saving JSON file: " + e.Message);
        }
    }
}
