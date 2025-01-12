using UnityEngine;
using TMPro;
using UnityEngine.UI;  // Add UI namespace

public class PointerController : MonoBehaviour
{
    // TextMeshProUGUI array for getting 8 service values
    public TextMeshProUGUI[] serviceValueTexts; // Eight TextMeshProUGUI objects

    // Current selection (range: 1-8)
    [Range(1, 8)]
    public int currentChoice = 1; // Initial selection is 1

    private int previousChoice = 1; // Record previous selection value

    // Current rotation angle (Z axis)
    private float currentZRotation;

    // Target rotation angle (Z axis)
    private float targetZRotation;

    // Rotation speed
    public float rotationSpeed = 100f; // Default rotation speed

    // Rotation center point
    public Transform rotationCenter; // Use Transform as center point object

    // TextMeshPro for displaying service value
    public TextMeshPro serviceValueText; // Bind a TextMeshProUGUI component

    // Button array for binding each button
    public Button[] buttons; // Eight buttons

    void Start()
    {
        // Check and auto-create center point (if not set)
        if (rotationCenter == null)
        {
            rotationCenter = new GameObject("RotationCenter").transform; // Dynamically create center point
            rotationCenter.position = Vector3.zero; // Default position at origin
            Debug.LogWarning("Rotation center was not set. A new one has been created at (0,0,0).");
        }

        // Reset rotation center's initial rotation
        rotationCenter.rotation = Quaternion.identity;

        // Initialize target angle
        UpdateTargetAngle();

        // Set initial angle
        currentZRotation = targetZRotation;
        UpdatePointerRotation();

        // Initialize service value display
        UpdateServiceValueText();

        // Bind click events for each button
        for (int i = 0; i < buttons.Length; i++)
        {
            int index = i + 1; // Get button's corresponding selection value (starting from 1)
            buttons[i].onClick.AddListener(() => OnButtonClick(index));
        }
    }

    void Update()
    {
        // Check if current selection has changed
        if (currentChoice != previousChoice)
        {
            UpdateTargetAngle();
            UpdateServiceValueText();
            previousChoice = currentChoice;
        }

        // Smoothly update rotation
        RotatePointer();
    }

    // Button click event handler
    void OnButtonClick(int choice)
    {
        currentChoice = choice;  // Set current selection
    }

    // Update target rotation angle
    void UpdateTargetAngle()
    {
        // Get service value corresponding to current selection
        float serviceValue = GetServiceValue(currentChoice);

        // Calculate target rotation angle (-90 to 90)
        targetZRotation = (serviceValue * 180) - 90;

        Debug.Log($"Selection: {currentChoice}, Service Value: {serviceValue}, Target Z Angle: {targetZRotation}");
    }

    // Smoothly update rotation
    void RotatePointer()
    {
        if (Mathf.Abs(currentZRotation - targetZRotation) > 0.01f)
        {
            // Smoothly transition current angle to target angle
            currentZRotation = Mathf.Lerp(
                currentZRotation,
                targetZRotation,
                Time.deltaTime * Mathf.Clamp(rotationSpeed / 100f, 0.1f, 10f)
            );

            // Update object rotation
            UpdatePointerRotation();
        }
    }

    // Update rotation based on current angle
    void UpdatePointerRotation()
    {
        if (rotationCenter != null)
        {
            // Only update rotation center's Z axis rotation
            rotationCenter.rotation = Quaternion.Euler(90 - currentZRotation, -90, 90);
        }
        else
        {
            Debug.LogWarning("Rotation center point not set!");
        }
    }

    // Update service value display
    void UpdateServiceValueText()
    {
        if (serviceValueText != null)
        {
            // Get service value corresponding to current selection
            float serviceValue = GetServiceValue(currentChoice);

            // Update TextMeshPro text
            serviceValueText.text = $"{serviceValue:F2}";
        }
        else
        {
            Debug.LogWarning("ServiceValueText not bound! Please bind a TextMeshProUGUI component.");
        }
    }

    // Get service value corresponding to current selection
    float GetServiceValue(int choice)
    {
        if (choice >= 1 && choice <= serviceValueTexts.Length)
        {
            // Get text from corresponding TextMeshProUGUI and convert to float
            float serviceValue = 0.5f; // Default value
            if (float.TryParse(serviceValueTexts[choice - 1].text, out serviceValue))
            {
                return serviceValue;
            }
            else
            {
                Debug.LogWarning($"Cannot parse text {serviceValueTexts[choice - 1].text} to float, using default value 0.5");
                return 0.5f;
            }
        }
        return 0.5f; // Return default value if selection is out of range
    }
}
