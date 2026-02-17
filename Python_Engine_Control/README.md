# Tiva C CAN-UART Engine Control Bridge

## Project Overview
This project implements a bidirectional control system using two Tiva C (TM4C123) microcontrollers. It demonstrates a physically decoupled transport layer where a **Gateway** node bridges PC-based commands to a **Plant** node over a CAN bus, with real-time telemetry visualized on a Python dashboard.

## System Architecture
The system consists of two primary hardware nodes and a software controller:

### 1. Controller (Python Dashboard)
- **Role:** User interface and data logger.
- **Communication:** UART @ 115200 Baud.
- **Features:** Tkinter-based GUI with real-time Matplotlib plotting for RPM and Throttle levels.

### 2. Gateway Node (Tiva C #1)
- **Role:** Protocol Bridge.
- **Functionality:** - Translates UART commands from the PC into **CAN messages** (ID: `0x200`).
  - Listens for **CAN telemetry** (ID: `0x101`) and forwards it to the PC as structured JSON strings.

### 3. Plant Node (Tiva C #2)
- **Role:** Physical System Simulation.
- **Functionality:** - Runs a dynamic physics engine that integrates throttle inputs into RPM.
  - Handles incoming CAN commands via high-priority interrupts.
  - Broadcasts engine state (RPM and Throttle Ack) back to the Gateway.

## Hardware Status Indicators
- **Blue LED (Plant):** Toggles on every successful CAN command received, providing a visual "heartbeat" of the control loop.
- **Red LED:** Indicates CAN bus errors or status issues (LEC).

## Current Project State: Milestone 0
- [x] **Bidirectional Communication:** Successfully verified data flow from Python -> Gateway -> CAN -> Plant -> CAN -> Gateway -> Python.
- [x] **Interrupt Logic:** Optimized `CAN0IntHandler` to prevent bus flooding and ensure stable telemetry streams.
- [x] **Real-time Plotting:** Dashboard correctly visualizes the logarithmic RPM curve in response to step throttle inputs.

## How to Run
1. Connect both Tiva C boards via their CAN transceiver headers.
2. Flash the `Gateway` firmware to Node A and the `Plant` firmware to Node B.
3. Connect the Gateway USB to your PC.
4. Run the dashboard:
   ```bash
   python src/dashboard.py