# Engine Control Node (Node B - Plant)

## Project Overview
The **Plant Node** is a Hardware-in-the-Loop (HIL) simulation of a combustion engine. Running on a Tiva C Series TM4C123, it simulates engine physics and communicates over a 500kbps CAN bus. It acts as the "Plant" in a control system, receiving throttle inputs and generating realistic engine telemetry.

## Functional Objectives
* **Physics Simulation:** Map throttle percentage (0-100%) to Engine RPM (800-8000 RPM).
* **Mechanical Inertia:** Simulate engine mass and friction so RPM changes are gradual rather than instantaneous.
* **Real-time Communication:** Listen for incoming commands and broadcast telemetry every 20ms.

## Software Architecture

| Module | Filename | Description |
| :--- | :--- | :--- |
| **Main Logic** | `plant.c` | Coordinates the 20ms execution loop and system initialization. |
| **Physics Engine** | `engine_sim.c` | Maintains engine state and implements the polynomial RPM curve. |
| **CAN Driver** | `can_handler.c` | Manages the CAN0 hardware, interrupts, and message object mailboxes. |

## State Management: Requested vs. Actual
The simulation maintains a distinction between the "Requested" state and the "Simulated" state to mimic real-world mechanical lag:

1.  **Requested Throttle (`g_ui8ThrottleCommand`):** The raw target received via CAN ID `0x200`. This represents the command from the Main Controller (Node A).
2.  **Target RPM:** Calculated via the polynomial: $RPM_{target} = 0.6T^2 + 10T + 800$.
3.  **Actual RPM (`current_rpm`):** The filtered value that "chases" the Target RPM based on the defined Inertia/Alpha factor.



## Data Structures & Variables
* **Incoming Command (ID 0x200):** Byte[0] = Throttle % (0-100).
* **Outgoing Telemetry (ID 0x101):** * Byte[0-1]: Engine RPM (16-bit, Big Endian).
    * Byte[2]: Current Throttle Command acknowledgment.

## Roadmap & Fault Injection
Future versions of this node will include "Real-world Faults" to test the robustness of the Main Controller:
* **Actuator Lag:** Introducing random delays in throttle response.
* **Signal Noise:** Adding $\pm 5$ RPM jitter to the telemetry output.
* **Stuck Actuator:** Randomly ignoring commands to simulate a mechanical throttle failure.