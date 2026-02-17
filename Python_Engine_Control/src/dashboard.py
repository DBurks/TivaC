import tkinter as tk
from tkinter import messagebox
import serial
import json
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque

# --- Configuration ---
SERIAL_PORT = 'COM4'  # Change this to your Gateway's COM port
BAUD_RATE = 115200

class EngineDash:
    def __init__(self, root):
        self.root = root
        self.root.title("Tiva C CAN Engine Controller")
        
        # Data storage for plotting (last 100 points)
        self.rpm_data = deque([0]*100, maxlen=100)
        self.throttle_data = deque([0]*100, maxlen=100)
        self.time_axis = list(range(100))
        
        # Serial Connection
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        except Exception as e:
            print(f"Error opening serial port: {e}")
            self.ser = None

        self.setup_ui()
        
        # Start background thread to read serial
        self.running = True
        self.thread = threading.Thread(target=self.read_serial, daemon=True)
        self.thread.start()

    def setup_ui(self):
        # Top Frame: Controls
        ctrl_frame = tk.Frame(self.root)
        ctrl_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=10)

        tk.Label(ctrl_frame, text="Set Throttle (0-100%):").pack(side=tk.LEFT)
        self.throttle_entry = tk.Entry(ctrl_frame, width=10)
        self.throttle_entry.pack(side=tk.LEFT, padx=5)
        
        self.send_btn = tk.Button(ctrl_frame, text="Update Engine", command=self.send_command)
        self.send_btn.pack(side=tk.LEFT)

        self.status_label = tk.Label(self.root, text="RPM: 0 | Throttle Ack: 0%", font=("Arial", 12, "bold"))
        self.status_label.pack(pady=5)

        # Bottom Frame: Plot
        self.fig, (self.ax_rpm, self.ax_thr) = plt.subplots(2, 1, figsize=(6, 4), sharex=True)
        self.fig.tight_layout(pad=3.0)
        
        self.line_rpm, = self.ax_rpm.plot(self.time_axis, self.rpm_data, color='blue')
        self.ax_rpm.set_ylabel("RPM")
        self.ax_rpm.set_ylim(0, 9000)

        self.line_thr, = self.ax_thr.plot(self.time_axis, self.throttle_data, color='red')
        self.ax_thr.set_ylabel("Throttle %")
        self.ax_thr.set_ylim(-5, 105)

        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

    def send_command(self):
        val = self.throttle_entry.get()
        print(f"DEBUG: Sending to Tiva: {val}")
        if val.isdigit():
            percent = int(val)
            if 0 <= percent <= 100:
                if self.ser:
                    self.ser.write(f"{percent}\n".encode())
            else:
                messagebox.showerror("Error", "Range must be 0-100")
        else:
            messagebox.showerror("Error", "Please enter a valid integer")

    def read_serial(self):
        while self.running:
            if self.ser and self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode('utf-8').strip()
                    if line.startswith('{') and line.endswith('}'):
                        data = json.loads(line)
                        rpm = data.get("rpm", 0)
                        throttle = data.get("throttle", 0)
                        
                        self.rpm_data.append(rpm)
                        self.throttle_data.append(throttle)
                        
                        self.root.after(0, self.update_gui, rpm, throttle)
    
                except json.JSONDecodeError:
                    continue # Ignore partial lines

    def update_gui(self, rpm, throttle):
        self.status_label.config(text=f"RPM: {rpm} | Throttle Ack: {throttle}%")
        self.line_rpm.set_ydata(self.rpm_data)
        self.line_thr.set_ydata(self.throttle_data)

        # AUTO-SCALE Y for RPM if it goes too high
        current_max = max(self.rpm_data)
        if current_max > self.ax_rpm.get_ylim()[1]:
            self.ax_rpm.set_ylim(0, current_max + 1000)
            
        self.canvas.draw()

if __name__ == "__main__":
    root = tk.Tk()
    app = EngineDash(root)
    root.mainloop()
