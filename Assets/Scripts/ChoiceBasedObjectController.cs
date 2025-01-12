using System;
using TMPro;
using UnityEngine;
using System.Text.RegularExpressions;

public class ChoiceBasedObjectController : MonoBehaviour
{
    // 通过 PointerController 获取 currentChoice
    public PointerController pointerController;  // PointerController 脚本的引用

    // 公用 TextMeshPro 组件
    public TextMeshPro serviceValueText; // 显示服务值 (修正为 TextMeshPro)

    // 每个选择对应的时间文本（固定为8个选择）
    public TextMeshProUGUI[] timeValueText1 = new TextMeshProUGUI[8];  // 8个选择的时间文本

    // 五个物体（一个物体数组）
    public GameObject[] objectsToControl = new GameObject[5];  // 5个物体

    void Start()
    {
        // 初始化，隐藏所有物体
        HideAllObjects();
    }

    void Update()
    {
        // 只读取 PointerController 中的 currentChoice 来更新
        int currentChoice = pointerController.currentChoice;  // 从 PointerController 获取当前选择

        // 根据当前选择更新文本内容
        UpdateServiceValueText(currentChoice);

        // 检查当前选择，并根据对应的值来更新时间区间并显示物体
        if (currentChoice >= 1 && currentChoice <= 8)
        {
            HandleChoiceBasedOnTime(currentChoice);
        }
        else
        {
            serviceValueText.text = "Invalid choice value"; // 无效选择
            HideAllObjects(); // 隐藏所有物体
        }
    }

    // 根据当前选择更新服务值文本
    void UpdateServiceValueText(int currentChoice)
    {
        if (currentChoice < 1 || currentChoice > 8)
        {
            serviceValueText.text = "Invalid choice value"; // 无效选择
            HideAllObjects(); // 隐藏所有物体
            return;
        }

        // 检查 timeValueText1 的值是否为 -1
        string timeText1 = timeValueText1[currentChoice - 1].text;
        float timeValue1 = ExtractTimeValue(timeText1);

        if (timeValue1 == -1)
        {
            serviceValueText.text = "Wait for more data"; // 显示等待数据的提示
            HideAllObjects(); // 隐藏所有物体
            return;
        }

        // 更新 serviceValueText 根据 currentChoice
        switch (currentChoice)
        {
            case 1:
                serviceValueText.text = "Central-Inbound";
                break;
            case 2:
                serviceValueText.text = "Central-Outbound";
                break;
            case 3:
                serviceValueText.text = "DLR-Inbound";
                break;
            case 4:
                serviceValueText.text = "DLR-Outbound";
                break;
            case 5:
                serviceValueText.text = "Elizabethline-Inbound";
                break;
            case 6:
                serviceValueText.text = "Elizabethline-Outbound";
                break;
            case 7:
                serviceValueText.text = "Jubilee-Inbound";
                break;
            case 8:
                serviceValueText.text = "Jubilee-Outbound";
                break;
        }

        // 隐藏所有物体并根据时间值显示相应物体
        HandleChoiceBasedOnTime(currentChoice);
    }

    // 处理选择并根据时间值显示对应物体
    void HandleChoiceBasedOnTime(int currentChoice)
    {
        // 检查 currentChoice 是否在有效范围内
        if (currentChoice < 1 || currentChoice > 8)
        {
            Debug.LogWarning("Invalid choice. currentChoice must be between 1 and 8.");
            HideAllObjects();
            return;
        }

        // 从 `PointerController` 获取时间文本的数值
        string timeText1 = timeValueText1[currentChoice - 1].text;

        // 清除文本中的非数字字符
        float timeValue1 = ExtractTimeValue(timeText1);

        // 隐藏所有物体
        HideAllObjects();

        // 如果时间值不为 -1，判断选择并显示对应物体
        if (timeValue1 != -1)
        {
            // 判断时间值是否在指定范围内，并显示对应的物体
            ShowObjectBasedOnRange(timeValue1);
        }
    }

    // 提取时间值并处理文本中的非数字部分
    float ExtractTimeValue(string text)
    {
        // 1. 移除所有非数字和小数点字符
        string cleanedText = Regex.Replace(text, @"[^\d.-]", "");

        // 2. 尝试将清理后的文本转换为浮动数值
        float result = 0f;
        if (float.TryParse(cleanedText, out result))
        {
            return result;
        }
        else
        {
            // 如果无法解析，返回默认值（可以是0或其他）
            Debug.LogWarning($"无法解析时间文本: {text}");
            return -1f; // 返回无效值
        }
    }

    // 根据时间值显示对应的物体
    void ShowObjectBasedOnRange(float timeValue)
    {
        // 判断 timeValue 是否在指定的范围内并显示物体
        if (IsInRange(timeValue))
        {
            int objectIndex = GetObjectIndexByRange(timeValue);

            // 显示对应物体
            if (objectIndex != -1)
                objectsToControl[objectIndex].SetActive(true);
        }
    }

    // 判断时间值是否在指定范围内
    bool IsInRange(float value)
    {
        return (value >= -0.1f && value <= 5.1f) ||
               (value > 5.1f && value <= 10.1f) ||
               (value > 10.1f && value <= 15.1f) ||
               (value > 15.1f && value <= 20.1f) ||
               (value > 20.1f && value <= 25.1f);
    }

    // 获取物体的索引，根据时间值的区间
    int GetObjectIndexByRange(float value)
    {
        if (value >= -0.1f && value <= 5.1f) return 0;
        if (value > 5.1f && value <= 10.1f) return 1;
        if (value > 10.1f && value <= 15.1f) return 2;
        if (value > 15.1f && value <= 20.1f) return 3;
        if (value > 20.1f && value <= 25.1f) return 4;
        return -1; // 无效区间
    }

    // 隐藏所有物体
    void HideAllObjects()
    {
        foreach (var obj in objectsToControl)
        {
            obj.SetActive(false);
        }
    }
}
