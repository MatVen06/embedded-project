import queue
import serial
import time
import threading
import serial.tools.list_ports
from PyQt5 import QtWidgets, QtCore
from collections import deque
import pyqtgraph as pg
import sounddevice as sd
import numpy as np

data_queue = queue.Queue()
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1.0)
ser.reset_input_buffer()
app = QtWidgets.QApplication([])
win = pg.plot(title="STM32 Telemetry Monitor")
win.setLabel('left', 'Temperature', units='°C')
win.setLabel('bottom', 'Time')
win.showGrid(x=True, y=True)
win.addLegend()
curve_pot = win.plot(pen = 'b', name = 'Potentiometer')
curve_ntc = win.plot(pen = 'r', name = 'NTC Thermistor')
pot_data = deque(maxlen=200)
ntc_data = deque(maxlen=200)
time_data = deque(maxlen=200)


def serial_reader_worker(ser: serial.Serial, data_queue: queue.Queue) :
    while ser.is_open:
        try:
            raw_line = ser.readline()
            line = raw_line.decode('utf-8', errors='ignore').strip()
        except (serial.SerialException, TypeError, OSError):
            break
        if not line :
            continue

        if "POT_" in line and "NTC_" in line:
            parts = line.split(',')
            if len(parts) == 4 :
                try : 
                    telemetry_data = {}
                    for item in parts :
                        key, val = item.split(':')
                        key = key.strip()
                        val = val.strip()
                        if "AVG" in key or "RAW" in key :
                            telemetry_data[key] = int(val)
                        else :
                            telemetry_data[key] = float(val)
                        

                    data_queue.put(telemetry_data)
                except (ValueError, IndexError):
                    print("malformed packet skipped")
                    continue
            else:
                pass
        else:
            print("[STATUS]:", line)


                
thread = threading.Thread(target=serial_reader_worker, args=(ser, data_queue), daemon=True)
thread.start()
timer =QtCore.QTimer()
sample_count = 0
current_diff = 0.0
def update_plot():
    global sample_count
    global current_diff
    while not data_queue.empty():
        data = data_queue.get_nowait()
        pot_value = data.get('POT_C')
        ntc_value = data.get('NTC_C')

        if pot_value is not None and ntc_value is not None:
            pot_data.append(pot_value)
            ntc_data.append(ntc_value)
            time_data.append(sample_count)
            sample_count += 1
            current_diff = max(0.0, ntc_value - pot_value)

    if pot_data:
        curve_pot.setData(list(time_data), list(pot_data))
    if ntc_data:
        curve_ntc.setData(list(time_data), list(ntc_data))


SAMPLE_RATE = 44100
current_phase = 0.0
BASE_FREQ = 220.0
MAX_FREQ = 1200
FREQ_SCALE = 65.0
raw_frequency = 0.0
frequency = 0.0
smoothed_freq = BASE_FREQ
def audio_callback(outdata, frames, time, status):
    global current_phase
    global temperature_span
    global raw_frequency
    global smoothed_freq


    raw_frequency =  BASE_FREQ+ (FREQ_SCALE * current_diff)

    frequency = min(MAX_FREQ, raw_frequency)
    smoothed_freq = smoothed_freq + 0.2*(frequency-smoothed_freq)
    delta_phi = (2*np.pi*smoothed_freq)/SAMPLE_RATE
    np.arange(frames)
    angles = current_phase + (np.arange(frames) * delta_phi)
    outdata[:,0] = 0.05 * np.sin(angles)
    current_phase = (current_phase + frames * delta_phi) % (2*np.pi)


stream = sd.OutputStream(samplerate=SAMPLE_RATE, channels=1, callback=audio_callback)

stream.start()
timer.timeout.connect(update_plot)
timer.start(30)

app.exec_()
stream.stop()
stream.close()
ser.close()
thread.join(timeout=1.0)