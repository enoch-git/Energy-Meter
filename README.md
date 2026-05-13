# Energy-Meter (AMI-V0.1)
# AMI-V0.1: Dual-Channel Smart Energy Meter
**An open-architecture edge-computing node for real-time power profiling and cost tracking.**

![Project Status](https://img.shields.io/badge/Status-Active_Development-brightgreen)
![Platform](https://img.shields.io/badge/Platform-ESP32--C3-blue)
![RTOS](https://img.shields.io/badge/OS-FreeRTOS-orange)
![Cloud](https://img.shields.io/badge/Cloud-ThingsBoard-blueviolet)

## Overview
In dynamic energy environments where facilities constantly switch between the main grid, solar inverters, and backup generators, standard utility meters fail to provide actionable insights. 

**AMI-V0.1** (Advanced Metering Infrastructure) by Lytenergy Systems is a localized IoT monitoring node designed to solve this. Built on the ESP32-C3 RISC-V architecture, this device physically clips onto multiple power feeds to act as an intelligent, high-speed watchdog. It calculates true energy consumption, tracks tariff burn rates in real-time, and provides cloud-based telemetry without dropping data during internet outages.

##  Key Features
* **Dual-Source Power Profiling:** Intelligently compares two isolated current channels (e.g., Grid vs. Local Inverter) to determine the active power source and log exact generator running hours.
* **Edge-Computed DSP:** Utilizes an Exponential Moving Average (EMA) filter to clean noisy ADC signals at 2kHz, ensuring completely stable, commercial-grade telemetry.
* **Real-Time Financial Tracking:** Translates raw Watt-hours into actual monetary spend using dynamic cloud-synced tariff rates, preventing bill shock.
* **Overload Watchdog:** A preemptive FreeRTOS alarm task that detects load spikes and pushes instantaneous MQTT alerts before physical hardware (like battery banks or inverters) trips.
* **Over-The-Air (OTA) Configuration:** Calibration factors, assumed voltages, and tariff rates can be updated instantly from the cloud dashboard without re-flashing the firmware.

##  Hardware Stack
* **Microcontroller:** ESP32-C3 SuperMini (Single-Core, 160MHz)
* **Current Sensors:** 2x SCT-013 (100A/50mA) Split-Core Transformers
* **Power Supply:** Onboard Hi-Link HLK-PM01 (230V AC to 5V DC)
* **Analog Front End:** Custom 1.65V DC-biased voltage divider network

##  Software Architecture
The firmware is built in **C++** using **PlatformIO** and heavily relies on **FreeRTOS** for deterministic task scheduling. 
* `Core 0 (High Priority)`: Dedicated to non-blocking ADC sampling, RMS math, and DSP filtering to ensure accurate waveform capture.
* `Core 0 (Low Priority)`: Handles WiFi persistence, ThingsBoard MQTT JSON serialization, and listening for remote RPC commands.

##  Getting Started

### Prerequisites
* [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO Extension](https://platformio.org/)
* A free [ThingsBoard](https://demo.thingsboard.io/) account.

### Installation
1. Clone this repository: `git clone https://github.com/YourOrg/AMI-V0.1.git`
2. Open the project folder in VS Code.
3. Open `src/main.cpp` and update your WiFi credentials and `TB_TOKEN` (ThingsBoard Access Token).
4. Connect your ESP32-C3 via USB-C and click the **PlatformIO: Upload** arrow in the bottom status bar.
5. The device will automatically provision itself to your ThingsBoard dashboard.

---
*Designed and engineered for smarter, more resilient micro-grids.*