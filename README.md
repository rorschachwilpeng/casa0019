# CASA0019: Sensor Data Visualisation 24/25: SubRadar
<p align="center">
  <img src="image/page_info.png" alt="NOVA" width="800">
</p>

## Table of Contents
1. [Project Overview](#project--overview)
2. [Dataset](#dataset)
3. [Data Device (Physical&Digital)](#data-device)
4. [How to use](#how-to-use)

## Project Overview
The **SubRadar** project is a sensor-based system that visualizes real-time transportation data. Combining physical and digital elements, it provides train arrival times, service quality, and directional updates. Users interact with the device through a rotary knob, button, and LED indicators.

## Dataset
**Data Source: [TFL Tube Data](https://api.tfl.gov.uk/swagger/ui/index.html?url=/swagger/docs/v1#!/Line/Line_Arrivals)**

*Used Data:*
| **Field Name**       | **Meaning**                                | **Purpose**                                                                                         |
|-----------------------|--------------------------------------------|-----------------------------------------------------------------------------------------------------|
| `lineName`           | Line name                                  | Used to distinguish data for different lines                                                       |
| `expectedArrival`    | Scheduled arrival time                     | Used to calculate service interval variability                                                     |
| `timeToStation`      | Remaining time to arrival                  | Used to calculate passenger waiting time                                                           |
| `direction`          | Train direction                            | Used to distinguish service levels for different directions                                        |

*Output Data:*
| **Field Name**       | **Meaning**                                | **Purpose**                                                                                         |
|-----------------------|--------------------------------------------|-----------------------------------------------------------------------------------------------------|
| `timeToStation`           | Time for the nearest tube to the station                                 | Notify user                                                       |
| `serviceLevel`    | Line's overall service level(50%*Passenger Waiting Time + 50%*Service Interval Variability)                    |                  Demonstrate the line's service level                                    |







## Data Device
### DATA DEVICE (Physical)
Color-Coded Indicators and Pointer: The device visually represents the service status of different lines (ranging from "Bad Service" to "Good Service") using a color-coded system and a swinging pointer.

Knob and Button Interaction: Users can rotate the knob to select specific line information and press the button to switch the train direction between Westbound and Eastbound.

LED Display: LED lights on the pointer display the arrival time (in minutes) of the next train, allowing users to quickly assess the waiting time.

### DATA DEVICE (Digital)
Real-Time Station Information: A digital screen displays detailed station names for the user-selected line, along with real-time updates on service quality.

Arrival Time: The device screen updates the arrival time of the next train at the nearest station to ensure timely information.

Service Quality: The overall service quality of the selected line is visually represented as a percentage, giving users a clear understanding of the line’s operational status.

### 

## HOW TO USE
Launch the App: Open the app and point it at the screen of the data device. Augmented Reality (AR) information will be overlaid on the data device.