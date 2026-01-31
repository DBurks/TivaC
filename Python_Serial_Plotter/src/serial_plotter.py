import serial
import json
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# serial confiuguration data
PORT = "COM4"
BAUD = 115200

# Data storage
x_data = []
y_data = []
colors = []

# Alarm Type mapping
ALARM_MAP = {0: "green", 1: "blue", 2: "red"}


# Setup the serial connection
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(f"Connected to {PORT} at {BAUD} baud.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit()


# Setup the plot
fig, ax = plt.subplots()
(line,) = ax.plot([], [], lw=2)
ax.set_ylim(10, 40)
ax.set_xlabel("Ticks")
ax.set_ylabel("Temperature (°C)")
ax.set_title("Real-time Temperature Data with Alarms")


def update(frame):
    if ser.in_waiting > 0:
        raw_line = ser.readline().decode("utf-8").strip()
        try:
            # Parse the json data
            data = json.loads(raw_line)
            x_data.append(data["time"])
            y_data.append(data["temp"])
            current_alarm = data["alarm"]
            ax.set_title(
                f"Temp: {data['temp']}", color=ALARM_MAP.get(current_alarm, "black")
            )

            if len(x_data) > 50:
                x_data.pop(0)
                y_data.pop(0)

            ax.set_xlim(max(0, x_data[0]), x_data[-1] + 1)
            line.set_data(x_data, y_data)
            line.set_color(ALARM_MAP.get(current_alarm, "green"))

        except (json.JSONDecodeError, KeyError):
            pass  # skip malformed data

    return (line,)


ani = FuncAnimation(fig, update, interval=50, cache_frame_data=False)
plt.show()
ser.close()
print("Serial connection closed.")
