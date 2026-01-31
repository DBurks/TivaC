import tkinter as tk
from tkinter import ttk
import serial
import json
import threading
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection
from matplotlib.colors import ListedColormap, BoundaryNorm
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation


class TelemetryApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Telemetry Data Plotter")

        # --- Data State ---
        self.x_data = []
        self.y_data = []
        self.max_threshold = 30.0
        self.min_threshold = 20.0
        self.running = True

        # --- Serial Configuration ---
        try:
            self.ser = serial.Serial("COM4", 115200, timeout=0.1)
        except Exception as e:
            print(f"Error opening serial port: {e}")
            self.ser = None

        # --- UI Layout ---
        self.setup_ui()

        # --- Background Thread for Serial ---
        self.serial_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.serial_thread.start()

        # --- Animation ---
        self.ani = FuncAnimation(
            self.fig, self.update_plot, interval=50, cache_frame_data=False
        )

        # --- Attach Window Exit Handler ---
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def on_close(self):
        """Stops the serial thread and closes the window properly"""
        print("Shutdown Signal received. Closing serial port...")
        # 1. Signal thread to stop
        self.running = False

        # 2. close the serial to break readline calls
        if self.ser and self.ser.is_open:
            self.ser.close()

        # 3. Wait for thread to die
        if hasattr(self, "serial_thread") and self.serial_thread.is_alive():
            self.serial_thread.join(timeout=1.0)

        # 4. Stop the animation
        if hasattr(self, "ani"):
            self.ani.event_source.stop()

        # 5. Destroy the window
        self.root.quit()
        self.root.destroy()
        print("Exit scucessful")

    def setup_ui(self):
        # Sidebar for controls
        sidebar = ttk.Frame(self.root, padding="10")
        sidebar.pack(side=tk.RIGHT, fill=tk.Y)

        ttk.Label(
            sidebar, text="Threshold Controls", font=("Helvetica", 12, "bold")
        ).pack(pady=10)

        # Max Threshold Slider
        ttk.Label(sidebar, text="Max Temp (Red Alarm)").pack()
        self.max_slider = ttk.Scale(
            sidebar, from_=25, to=45, orient=tk.HORIZONTAL, command=self.update_max
        )
        self.max_slider.set(self.max_threshold)
        self.max_slider.pack(pady=5)

        # Min Threshold Slider
        ttk.Label(sidebar, text="Min Temp (Blue Alarm)").pack()
        self.min_slider = ttk.Scale(
            sidebar, from_=10, to=25, orient=tk.HORIZONTAL, command=self.update_min
        )
        self.min_slider.set(self.min_threshold)
        self.min_slider.pack(pady=5)

        # Plot Area
        self.fig, self.ax = plt.subplots(figsize=(6, 4), dpi=100)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    def update_max(self, val):
        self.max_threshold = float(val)
        if self.ser:
            self.ser.write(f"M{int(self.max_threshold)}\n".encode())

    def update_min(self, val):
        self.min_threshold = float(val)
        if self.ser:
            self.ser.write(f"L{int(self.min_threshold)}\n".encode())

    def read_serial(self):
        while self.running:
            if self.ser and self.ser.in_waiting:
                line = self.ser.readline().decode("utf-8").strip()
                if line.startswith("{"):
                    try:
                        data = json.loads(line)
                        self.x_data.append(data["time"])
                        self.y_data.append(data["temp"])
                        if len(self.x_data) > 50:
                            self.x_data.pop(0)
                            self.y_data.pop(0)
                    except (json.JSONDecodeError, KeyError):
                        pass

    def update_plot(self, frame):
        # app is shutting down so don;t redraw
        if not self.running or not self.root.winfo_exists():
            return

        if not self.x_data:
            return

        self.ax.clear()

        oldest_time = self.x_data[0]
        newest_time = self.x_data[-1]

        margin = 2
        self.ax.set_xlim(oldest_time - margin, newest_time + margin)

        self.ax.set_ylim(10, 50)
        self.ax.grid(True, alpha=0.3)

        # 1. Create Points and Segments
        points = np.array([self.x_data, self.y_data]).T.reshape(-1, 1, 2)
        segments = np.concatenate([points[:-1], points[1:]], axis=1)

        # 2. Create a colormap and norm based on thresholds
        cmap = ListedColormap(["blue", "green", "red"])
        norm = BoundaryNorm([0, self.min_threshold, self.max_threshold, 100], cmap.N)

        # 3. Create LineCollection
        lc = LineCollection(segments, cmap=cmap, norm=norm)
        lc.set_array(np.array(self.y_data))
        lc.set_linewidth(2)

        # 4. Add LineCollection to Axes
        self.ax.add_collection(lc)

        # Draw Threshold lines
        self.ax.axhline(self.max_threshold, color="red", linestyle="--", alpha=0.5)
        self.ax.axhline(self.min_threshold, color="blue", linestyle="--", alpha=0.5)

        # Plot the data
        # Note: We will implement segmented coloring in the next step
        self.ax.plot(self.x_data, self.y_data, color="gray", alpha=0.5)

        # Scatter points with conditional colors
        for x, y in zip(self.x_data, self.y_data):
            color = "green"
            if y >= self.max_threshold:
                color = "red"
            elif y <= self.min_threshold:
                color = "blue"
            else:
                color = "green"
            self.ax.scatter(x, y, color=color, s=15)

        self.ax.set_xlabel("Ticks")
        self.ax.set_ylabel("Temperature (°C)")
        self.ax.set_title(f"Live Temp: {self.y_data[-1]:.1f} °C")


if __name__ == "__main__":
    root = tk.Tk()
    app = TelemetryApp(root)
    root.mainloop()
