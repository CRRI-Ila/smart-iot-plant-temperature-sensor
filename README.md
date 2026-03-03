# ESP32 IoT Plant Monitoring and Smart Actuation System

## Overview

<img width="960" height="528" alt="image" src="https://github.com/user-attachments/assets/db1952ff-4727-492b-a1fd-55e2cabb1e3b" />

This project is a fully implemented ESP32-based IoT plant monitoring and smart actuation system. It continuously monitors environmental conditions such as temperature and brightness, publishes telemetry data via MQTT, and receives cloud-based control commands to manage heating or cooling mechanisms.

The system integrates embedded hardware control, real-time sensor acquisition, MQTT communication, JSON data structuring, and cloud-to-device actuation in a complete closed-loop IoT architecture.

---

## System Architecture

The system operates as follows:

ESP32  
→ Reads analog temperature sensor (GPIO35)  
→ Reads LDR brightness sensor (GPIO34)  
→ Processes environmental conditions  
→ Displays real-time data on OLED (SSD1306)  
→ Publishes structured JSON telemetry via MQTT  
→ Cloud platform (MQTT broker / Node-RED)  
→ Sends downlink command  
→ Controls relay, LEDs, and buzzer  

This creates a full device-to-cloud-to-device feedback loop.

---

## Hardware Components

- ESP32 Development Board  
- Analog temperature sensor connected to GPIO35  
- LDR brightness sensor connected to GPIO34  
- SSD1306 OLED display (I2C interface)  
- Relay module (GPIO5)  
- Buzzer (GPIO4)  
- Dual LED indicator (GPIO18 and GPIO19)  
- Status LED (GPIO23)  
- Mode selection push button (GPIO25)  

---

## Core Features

- Real-time temperature monitoring  
- Brightness monitoring with growth-light evaluation logic  
- OLED display for live system feedback  
- MQTT publish and subscribe communication  
- Cloud-controlled relay for heater or cooling fan  
- Buzzer alert feedback  
- Dual LED lighting condition indication  
- Plant mode selection using button input  
- Non-blocking timed publishing cycle  

---

## MQTT Communication

### Publish Topic

<appname>/<clientid>/<device>

### Subscribe Topic (Downlink)

DL/<appname>/<clientid>/<device>

---

## JSON Telemetry Structure

The device publishes structured JSON containing:

- v → temperature value (scaled)  
- l → brightness value (scaled)  
- m → plant mode selection  
- x → system state ("hot", "cold", "ideal")  
- r → relay status  

Example:

{
  "v": 2530,
  "l": 780,
  "m": 1,
  "x": "ideal",
  "r": 0
}

---

## Environmental Logic

### Growth Lighting Logic

Brightness is evaluated and categorized:

- Too dark → Both LEDs indicate insufficient lighting  
- Moderate brightness → Growth light warning  
- Bright enough → No growth light required  

### Temperature and Relay Control Logic

If a downlink command is received:

COOLING FAN ON  
→ Relay activated  
→ Buzzer alert  
→ State set to "hot"

HEATER ON  
→ Relay activated  
→ Double buzzer alert  
→ State set to "cold"

Any other message  
→ Relay deactivated  
→ State set to "ideal"

---

## Software Requirements

- Arduino IDE  
- ESP32 Board Package  
- PubSubClient library  
- ArduinoJson library  
- Adafruit SSD1306 library  
- Adafruit GFX library  

---

## Security Configuration

Secure MQTT (TLS) is supported via WiFiClientSecure using root CA certificates defined in cert.h.

Sensitive information is separated into configuration header files:

- credentials.h  
- parameters.h  
- profile.h  
- cert.h  

These files should not be committed to public repositories.

---

## Conclusion

This project demonstrates a complete IoT implementation including:

- Embedded sensor data acquisition  
- Real-time OLED interface  
- MQTT-based cloud communication  
- Structured JSON telemetry  
- Remote actuation via downlink commands  
- Closed-loop environmental control  

The system is fully functional and integrates hardware, firmware, networking, and cloud logic into a cohesive IoT solution.
