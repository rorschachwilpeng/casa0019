using UnityEngine;
using TMPro;
using System;

public class TimeUpdater : MonoBehaviour
{
    public TMP_Text[] timeInputs;   // Array of text components containing minute increments (e.g., 8, 10)
    public TMP_Text[] timeOutputs;  // Array of text components for displaying updated times

    void Update()
    {
        // Ensure both arrays have the same length
        if (timeInputs.Length == timeOutputs.Length)
        {
            // Get current time
            DateTime currentTime = DateTime.Now;

            // Iterate through each input text
            for (int i = 0; i < timeInputs.Length; i++)
            {
                // Get minute increment (from timeInputs)
                int minutesToAdd;
                if (int.TryParse(timeInputs[i].text, out minutesToAdd))
                {
                    // Calculate new time by adding minutes
                    DateTime updatedTime = currentTime.AddMinutes(minutesToAdd);

                    // Convert updated time to string and display in output text
                    string timeString = updatedTime.ToString("HH:mm");
                    timeOutputs[i].text = timeString;
                }
            }
        }
        else
        {
            Debug.LogError("timeInputs and timeOutputs arrays length mismatch!");
        }
    }
}
