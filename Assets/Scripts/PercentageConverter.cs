using TMPro;
using UnityEngine;

public class PercentageConverter : MonoBehaviour
{
    // Original text components
    public TMP_Text[] decimalTexts;  // Array for storing decimal texts
    // Array for storing converted percentage texts
    public TMP_Text[] percentageTexts; // Array for storing percentage texts

    void Update()
    {
        // Iterate through all decimal texts
        for (int i = 0; i < decimalTexts.Length; i++)
        {
            if (decimalTexts[i] != null)
            {
                // Get decimal value from text
                float decimalValue = 0f;
                if (float.TryParse(decimalTexts[i].text, out decimalValue))
                {
                    // Convert decimal value to percentage
                    string percentageText = (decimalValue * 100f).ToString() + "%";
                    // Display converted percentage in another text component
                    if (percentageTexts[i] != null)
                    {
                        percentageTexts[i].text = percentageText;
                    }
                }
            }
        }
    }
}
