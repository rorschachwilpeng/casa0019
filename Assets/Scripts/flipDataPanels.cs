using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class flipDataPanels : MonoBehaviour
{
    public GameObject[] visiblePanels;
    public GameObject[] hiddenPanels;

    // Start is called before the first frame update
    void Start()
    {
        for (int i = 0; i < hiddenPanels.Length; i++) {
            hiddenPanels[i].SetActive(false);
        }
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void flipToData(int index) {
        for (int i = 0; i < hiddenPanels.Length; i++) {
            if (i == index) {
                hiddenPanels[i].SetActive(true);
                visiblePanels[i].SetActive(false);
            }
            else {
                hiddenPanels[i].SetActive(false);
                visiblePanels[i].SetActive(true);
            }
        }
    }

    public void flipToMain(int index) {
        for (int i = 0; i < hiddenPanels.Length; i++) {
            if (i == index) {
                hiddenPanels[i].SetActive(false);
                visiblePanels[i].SetActive(true);
            }
            else {
                hiddenPanels[i].SetActive(false);
                visiblePanels[i].SetActive(true);
            }
        }
    }
}
