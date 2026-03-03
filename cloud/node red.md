



# Node-RED Flow Documentation (No WhatsApp) — ESP32 Temperature and Plant Mode Control

## Overview

<img width="575" height="267" alt="image" src="https://github.com/user-attachments/assets/ed0c7e2e-da41-4259-8cd4-e3eadd7a84f9" />

This Node-RED flow receives telemetry from an ESP32 over MQTT, transforms the payload into structured fields and tags, logs the data, and generates a downlink command back to the ESP32 for relay actuation (cooling fan / heater / off). It also includes a device switch node to support multiple devices (example: device S and device M) and a separate “Power mode” function to publish low/high power messages based on battery threshold.

This flow does not include any WhatsApp or external messaging.

---

## 1) MQTT Input Node (Telemetry In)

The flow starts with an MQTT In node subscribing to the ESP32 uplink topic.

Example uplink topic format:

TempSensor/<clientid>/<device>

Example:

TempSensor/57054/M

Incoming payload is JSON (from the ESP32 firmware), using short keys for efficiency.

Typical keys used:
- v  : temperature value (scaled)
- l  : brightness value (scaled)
- x  : state string (hot / cold / ideal)
- r  : relay status
- m  : plant mode (1 or 2)

The MQTT In node outputs the message into the debug node and into the Extract & Transform node.

---

## 2) Debug Node

A debug node is connected near the input to inspect:
- msg.topic
- msg.payload

This is used to confirm correct uplink topic formatting and payload correctness during testing.

---

## 3) Extract & Transform (Function Node)

<img width="420" height="566" alt="image" src="https://github.com/user-attachments/assets/8e01a6f1-46a7-4b70-9ff9-f73e3b47a22a" />

Purpose:
- Convert compact firmware keys into readable values
- Apply scaling conversions
- Extract device metadata from the MQTT topic
- Reformat msg.payload into a structured array containing fields and tags

Conversions shown in the function:
- Temperature scaling: msg.payload.v divided by 100
- Brightness scaling: msg.payload.l divided by 10
- State: from msg.payload.x
- Relay: from msg.payload.r
- PlantType/mode: from msg.payload.m
- clientid and device extracted from msg.topic by splitting using "/"

Output format (structured):

msg.payload = [
  {
    "Temp": <temperature>,
    "Brightness": <brightness>,
    "state": <state>,
    "relay": <relay>,
    "PlantType": <mode>
  },
  {
    "clientid": <clientid>,
    "device": <device>
  }
]

This makes downstream nodes (logging and actuation) consistent and easier to manage.

---

## 4) Switch Node (Device Routing)

<img width="401" height="302" alt="image" src="https://github.com/user-attachments/assets/60aad076-df54-4ee9-b85b-debaebb0504a" />

A switch node routes messages based on device ID:

Property used:
msg.payload[1].device

Rules:
- If device == "S" → output 1
- If device == "M" → output 2

This allows one Node-RED flow to handle multiple devices while still applying device-specific logic or logging.

---

## 5) Logging Path

A logging branch stores the processed telemetry into a database (commonly InfluxDB) for later visualization and history tracking.

Logged values typically include:
- Temp
- Brightness
- state
- relay
- PlantType
- tags: clientid, device
- timestamp (Node-RED time or device timestamp if provided)

This enables dashboards (Grafana) and long-term monitoring.

---

## 6) PowerMode (Function Node)

<img width="367" height="349" alt="image" src="https://github.com/user-attachments/assets/0d251379-0c48-46a7-ad1d-25f8ebd06ede" />

Purpose:
- Create a downlink topic based on the uplink topic
- Decide a simple power mode state based on battery threshold

Topic rewrite:
msg.topic = "DL/" + msg.topic

Decision logic:
- If msg.payload[0].vbat < 90 → msg.payload = "LO"
- Else → msg.payload = "HI"

This produces a power mode command that can be consumed by the ESP32 to reduce power usage or enable a higher performance state.

Note:
For PowerMode to work, vbat must exist in msg.payload[0]. If battery is not currently included, the firmware or Extract & Transform node should be updated to add vbat into the structured payload.

---

## 7) Actuation (Function Node)

<img width="374" height="553" alt="image" src="https://github.com/user-attachments/assets/93104a51-c750-4032-93af-5f429abb57b4" />

Purpose:
- Create the downlink topic
- Use PlantType and temperature thresholds to decide relay command for the ESP32

Topic rewrite:
msg.topic = "DL/" + msg.topic

Actuation decisions depend on PlantType:

PlantType == "1"
- If Temp > 30 → "COOLING FAN ON"
- Else if Temp < 26 → "HEATER ON"
- Else → "Off"

PlantType == "2"
- If Temp > 25 → "COOLING FAN ON"
- Else if Temp < 21 → "HEATER ON"
- Else → "Off"

This allows different plants (modes) to have different temperature control thresholds.

Output:
msg.payload is set to the final command string, which is then sent to MQTT Out.

---

## 8) MQTT Output Node (Downlink Out)

The flow publishes the command back to the ESP32 via MQTT Out.

Downlink topic format:

DL/TempSensor/<clientid>/<device>

Payload is a string, for example:
- "COOLING FAN ON"
- "HEATER ON"
- "Off"
- "HI"
- "LO"

The ESP32 receives this message and acts on it (relay control, buzzer, LEDs, OLED messaging).

---

## End-to-End Flow Summary

MQTT In (Telemetry)
  → Debug
  → Extract & Transform
  → Switch (route by device)
     → Logging (InfluxDB)
     → PowerMode (optional) → MQTT Out (DL)
     → Actuation (threshold logic) → MQTT Out (DL)

This completes a closed-loop IoT system:
ESP32 publishes telemetry → Node-RED processes and logs → Node-RED sends control commands → ESP32 actuates relay and updates UI.
```
