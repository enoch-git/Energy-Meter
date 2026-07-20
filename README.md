# AMI-V0.1: Edge-Computing Smart Energy Node
**An open-architecture, dual-channel IoT metering node engineered for real-time power profiling, dynamic tariff tracking, and non-residential OpEx optimization.**

<p align="center">
  <img src="https://img.shields.io/badge/Status-Active_Development-00C853?style=for-the-badge" alt="Project Status">
  <img src="https://img.shields.io/badge/Hardware-ESP32--C3_SuperMini-2962FF?style=for-the-badge" alt="Platform">
  <img src="https://img.shields.io/badge/OS-FreeRTOS-FF6D00?style=for-the-badge" alt="RTOS">
  <img src="https://img.shields.io/badge/Cloud-ThingsBoard_MQTT-AA00FF?style=for-the-badge" alt="Cloud">
  <img src="https://img.shields.io/badge/License-MIT-455A64?style=for-the-badge" alt="License">
</p>

---

## 🌐 Executive Overview
In infrastructure-challenged environments where commercial facilities constantly switch between the municipal grid, solar inverter banks, and localized backup generators, traditional passive utility meters fail to provide actionable operational insights. 

The **AMI-V0.1** (Advanced Metering Infrastructure) engineered by **Lytenergy Systems** bridges this critical data gap. Powered by the low-power RISC-V ESP32-C3 architecture and running a deterministic FreeRTOS kernel, this edge-computing node clips non-invasively onto dual power feeds. It continuously sanitizes noisy industrial waveforms, calculates true Root Mean Square (RMS) energy metrics, and pushes highly compressed JSON telemetry to the cloud—ensuring zero data loss during network dropouts while safeguarding commercial equipment against severe inductive load spikes.

---

## 📸 Hardware Architecture & Prototype

<p align="center">
  <img src="/test/SM/AMI2.jpg" alt="AMI-V0.1 Custom PCB and SCT-013 Current Transformer" width="600">
  <br>
  <em>Figure 1: The AMI-V0.1 hardware featuring the ESP32-C3 SuperMini, onboard 230V AC-to-5V DC HLK-PM01 power module, dual 3.5mm jack inputs, and custom 3D-printed enclosure with an SCT-013 split-core current transformer.</em>
</p>

---

## ⚡ Key Architectural Capabilities

* **Dual-Source Autonomous Profiling:** Continuously monitors two isolated analog channels (e.g., Grid vs. Backup Generator) to autonomously identify active power sources, log precise operational running hours, and flag fuel-wasting low-load generator states.
* **Edge-Computed Digital Signal Processing:** Bypasses cloud compute costs by executing an onboard Exponential Moving Average (EMA) filter at high sampling frequencies (2–5 kHz), eliminating industrial electromagnetic noise for commercial-grade RMS precision.
* **Real-Time OpEx & Tariff Tracking:** Dynamically translates raw energy consumption ($\text{Wh}$) into real-time financial burn rates using cloud-synced local currency tariffs, protecting small and medium enterprises (SMEs) from bill shock.
* **Preemptive Overload Watchdog:** Implements a dedicated FreeRTOS software watchdog that monitors for sustained inductive load surges, dispatching instantaneous MQTT alarms before fragile commercial solar inverters or circuit breakers trip.
* **Dynamic Cloud Configuration (OTA):** All local voltage baseline assumptions, calibration multipliers, and tariff pricing tiers can be remotely modified Over-The-Air via ThingsBoard shared attributes without technician dispatches or firmware re-flashing.

---

## 🛠️ Hardware Specification

The AMI-V0.1 is designed around accessible, commercial off-the-shelf (COTS) components integrated onto a custom layout to maintain an aggressively low Bill of Materials (BOM).

| Component | Model / Specification | Function |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-C3 SuperMini (RISC-V) | Single-core 160MHz CPU executing FreeRTOS and MQTT serialization. |
| **Current Sensors** | 2x SCT-013 (100A / 50mA) | Non-invasive split-core transformers for dual-channel load monitoring. |
| **Power Power Supply** | Hi-Link HLK-PM01 Module | Step-down isolation transformer converting 230V AC mains directly to 5V DC. |
| **Analog Front-End** | Custom Biasing Network | Precision resistor/capacitor network establishing a stable **1.65V DC bias** to safely shift alternating sine waves into the ADC range. |
| **Input Interfaces** | Dual 3.5mm Audio Jacks | Quick-release mechanical connections for external split-core CT clamps. |

---

## 💻 Firmware & FreeRTOS Task Scheduling

The firmware is developed in **C++** utilizing the **PlatformIO** ecosystem. To achieve high-precision signal processing on a single-core processor without radio interference, the system architecture relies on deterministic task time-slicing:

<p align="center">
  <img src="/test/SM/AMI1.jpg" alt="AMI-V0.1 Custom PCB and SCT-013 Current Transformer" width="600">
  <br>
```text
+-----------------------------------------------------------------+
|                       FreeRTOS Scheduler                        |
+-----------------------------------------------------------------+
        |                                         |
        v [High Priority Task]                    v [Low Priority Task]
+-------------------------------+       +-------------------------+
|      Task_SampleSensors       |       |        Task_MQTT        |
|-------------------------------|       |-------------------------|
| * Non-blocking ADC Reads      |       | * Wi-Fi Persistence     |
| * EMA DSP Waveform Filtering  | <---> | * JSON Serialization    |
| * True RMS Math & Accumulation|       | * ThingsBoard Pub/Sub   |
+-------------------------------+       +-------------------------+
