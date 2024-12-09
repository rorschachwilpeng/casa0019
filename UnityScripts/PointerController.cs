using UnityEngine;
using TMPro;

public class PointerController : MonoBehaviour
{
    private float[] serviceValues = { 0.23f, 0.7f, 0.84f, 0.4f, 0.5f, 0.31f };

    [Range(1, 6)]
    public int currentChoice = 1; 

    private int previousChoice = 1; 

    private float currentZRotation;

    private float targetZRotation;

    public float rotationSpeed = 100f; 

    public Transform rotationCenter; // Use Transform as the pivot point object

    // TextMeshPro display service value
    public TextMeshPro serviceValueText; 

    void Start()
    {
        if (rotationCenter == null)
        {
            rotationCenter = new GameObject("RotationCenter").transform; 
            rotationCenter.position = Vector3.zero; 
            Debug.LogWarning("Rotation center was not set. A new one has been created at (0,0,0).");
        }

        rotationCenter.rotation = Quaternion.identity;

        UpdateTargetAngle();

        currentZRotation = targetZRotation;
        UpdatePointerRotation();

        UpdateServiceValueText();
    }

    void Update()
    {
        if (currentChoice != previousChoice)
        {
            UpdateTargetAngle();
            UpdateServiceValueText();
            previousChoice = currentChoice;
        }

        RotatePointer();
    }

    void UpdateTargetAngle()
    {
        float serviceValue = serviceValues[currentChoice - 1];

        targetZRotation = (serviceValue * 180) - 90;

        Debug.Log($"Choice：{currentChoice}, ServiceValue：{serviceValue}, Target Z-axis angle：{targetZRotation}");
    }

    void RotatePointer()
    {
        if (Mathf.Abs(currentZRotation - targetZRotation) > 0.01f)
        {
            currentZRotation = Mathf.Lerp(
                currentZRotation,
                targetZRotation,
                Time.deltaTime * Mathf.Clamp(rotationSpeed / 100f, 0.1f, 10f)
            );

            UpdatePointerRotation();
        }
    }

    void UpdatePointerRotation()
    {
        if (rotationCenter != null)
        {
            rotationCenter.rotation = Quaternion.Euler(90 - currentZRotation, -90, 90);
        }
        else
        {
            Debug.LogWarning("No rotation pivot point is set！");
        }
    }

    void UpdateServiceValueText()
    {
        if (serviceValueText != null)
        {
            float serviceValue = serviceValues[currentChoice - 1];

            serviceValueText.text = $"{serviceValue:F2}";
        }
        else
        {
            Debug.LogWarning("ServiceValueText is not bound! Please bind a TextMeshProUGUI component.");
        }
    }
}
