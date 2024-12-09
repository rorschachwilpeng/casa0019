using System;
using TMPro;
using UnityEngine;
using System.Text.RegularExpressions;

public class ChoiceBasedObjectController : MonoBehaviour
{
    // get currentChoice by PointerController
    public PointerController pointerController;  

    // Public TextMeshPro Component
    public TextMeshPro serviceValueText; // Display the service value (corrected to TextMeshPro)

    // Two time texts corresponding to each selection (each selection has a set, using the TextMeshProUGUI type)
    public TextMeshProUGUI[] timeValueText1 = new TextMeshProUGUI[6];  // The first time text of the six selections
    public TextMeshProUGUI[] timeValueText2 = new TextMeshProUGUI[6];  // The second time text of the six selections

    // Five objects (an array of objects)
    public GameObject[] objectsToControl = new GameObject[5];  // 5 objects

    void Start()
    {
        // initialize, hide all objects
        HideAllObjects();
    }

    void Update()
    {
        // Only read the currentChoice from PointerController to update
        int currentChoice = pointerController.currentChoice;  

        // Update the text content based on the current selection
        UpdateServiceValueText(currentChoice);

        // Check the current selection and update the time range and display the object according to the corresponding value
        if (currentChoice >= 1 && currentChoice <= 6)
        {
            HandleChoiceBasedOnTime(currentChoice);
        }
        else
        {
            serviceValueText.text = "Invalid choice value"; // invalid selection
            HideAllObjects();
        }
    }

    // Update the service value text according to the current selection
    void UpdateServiceValueText(int currentChoice)
    {
        switch (currentChoice)
        {
            case 1:
                serviceValueText.text = "Central-Inbound-waiting";
                break;
            case 2:
                serviceValueText.text = "Central-Outbound-waiting";
                break;
            case 3:
                serviceValueText.text = "DLR-Inbound-waiting";
                break;
            case 4:
                serviceValueText.text = "DLR-Outbound-waiting";
                break;
            case 5:
                serviceValueText.text = "Elizabethline-Inbound-waiting";
                break;
            case 6:
                serviceValueText.text = "Elizabethline-Outbound-waiting";
                break;
        }
    }

    // Determine which objects to display based on the values of the time text
    void HandleChoiceBasedOnTime(int currentChoice)
    {
        // Retrieve the numerical values of the two time texts from `PointerController`
        string timeText1 = timeValueText1[currentChoice - 1].text;
        string timeText2 = timeValueText2[currentChoice - 1].text;

        // Remove non-numeric characters from the text
        float timeValue1 = ExtractTimeValue(timeText1);
        float timeValue2 = ExtractTimeValue(timeText2);

        HideAllObjects();

        // Determine the selection and display the corresponding object
        ShowObjectBasedOnRange(timeValue1, timeValue2);
    }

    // Extract the time value and process the non-numeric parts of the text
    float ExtractTimeValue(string text)
    {
        // 1. Remove all non-numeric and non-decimal point characters
        string cleanedText = Regex.Replace(text, @"[^\d.-]", "");

        // 2. Attempt to convert the cleaned text into a floating-point number
        float result = 0f;
        if (float.TryParse(cleanedText, out result))
        {
            return result;
        }
        else
        {
            // If it cannot be parsed, return the default value (which could be 0 or something else)
            Debug.LogWarning($"cannot parse: {text}");
            return -1f; // return invaid value
        }
    }

    // Display the corresponding object based on the time value
    void ShowObjectBasedOnRange(float timeValue1, float timeValue2)
    {
        // Determine the range between timeValue1 and timeValue2 and display the corresponding object
        if (IsInRange(timeValue1) && IsInRange(timeValue2))
        {
            int objectIndex1 = GetObjectIndexByRange(timeValue1);
            int objectIndex2 = GetObjectIndexByRange(timeValue2);

            // Display the corresponding object
            if (objectIndex1 != -1) objectsToControl[objectIndex1].SetActive(true);
            if (objectIndex2 != -1) objectsToControl[objectIndex2].SetActive(true);
        }
    }

    // Determine if the time value is within the specified range
    bool IsInRange(float value)
    {
        return (value >= -0.1f && value <= 5.1f) ||
               (value > 5.1f && value <= 10.1f) ||
               (value > 10.1f && value <= 15.1f) ||
               (value > 15.1f && value <= 20.1f) ||
               (value > 20.1f && value <= 25.1f);
    }

    // Get the index of the object, based on the range of time values
    int GetObjectIndexByRange(float value)
    {
        if (value >= -0.1f && value <= 5.1f) return 0;
        if (value > 5.1f && value <= 10.1f) return 1;
        if (value > 10.1f && value <= 15.1f) return 2;
        if (value > 15.1f && value <= 20.1f) return 3;
        if (value > 20.1f && value <= 25.1f) return 4;
        return -1; // invalid range
    }

    // hide all of the objects
    void HideAllObjects()
    {
        foreach (var obj in objectsToControl)
        {
            obj.SetActive(false);
        }
    }
}
