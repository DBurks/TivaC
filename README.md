# Tiva C Series Multi-Format UART Telemetry

A hands-on project for the **TM4C123GH6PM** (Tiva C) using **Code Composer Studio**. This project demonstrates how to bridge the gap between embedded firmware and high-level data consumption by outputting simulated temperature data in three distinct formats.

## 🚀 Project Roadmap
* **v0.1:** Basic UART Setup & "Hello World"
* **v0.2:** **(Current)** Human-Readable Dashboard (ANSI Escape Codes & Sinewave Simulation)
* **v0.3:** Machine-Readable Output (JSON/CSV)
* **v1.0:** Python GUI Serial Plotter Integration

---

## 📊 Milestone 2: The Human-Readable Dashboard

In this stage, we transition from a chaotic, scrolling "wall of text" to a clean, static dashboard within the serial terminal.

### Key Features
1.  **Sinewave Simulation:** Uses `math.h` to generate a temperature signal oscillating between 15°C and 35°C.
2.  **ANSI Control:** Uses escape sequences to manage terminal behavior:
    * `\x1b[2J`: Clears the screen.
    * `\x1b[H`: Moves the cursor to the "Home" position (top-left) to overwrite data without flickering.
3.  **Color-Coded Alarms:** * **Green:** Normal Operating Temp (20°C - 30°C)
    * **Blue:** Low Temp Alarm (< 20°C)
    * **Red:** High Temp Alarm (> 30°C)

### How to View
To see the formatting correctly, use a terminal emulator that supports **VT100/ANSI escape codes**:
* **Windows:** PuTTY, Tera Term, or the built-in VS Code Serial Monitor.
* **Linux/Mac:** `screen` or `minicom`.
* **Baud Rate:** `115200`

---

## 🛠️ Hardware & Setup
* **MCU:** TI Tiva C Series TM4C123G LaunchPad.
* **IDE:** Code Composer Studio (CCS).
* **Library:** TivaWare SDK.

### Build Instructions
1.  Clone the repo: 
    ```bash
    git clone [https://github.com/YOUR_USERNAME/TivaC-MultiFormat-UART.git](https://github.com/YOUR_USERNAME/TivaC-MultiFormat-UART.git)
    ```
2.  Import the project into CCS.
3.  Ensure your TivaWare path is linked in **Properties > ARM Compiler > Include Options**.
4.  Flash the MCU and open your serial terminal at 115200 baud.

---

## 📜 Development History (Tags)
You can check out the specific stages of this "episode" using git tags:
* `v0.1-uart-basic`: The simple scrolling "Hello World" state.
* `v0.2-human-readable`: The final colorized dashboard.

```bash
git checkout v0.2-human-readable