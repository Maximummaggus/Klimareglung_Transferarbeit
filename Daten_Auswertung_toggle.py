import tkinter as tk
from tkinter import ttk
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style
import datetime
import threading
import sys

# Globale Variable für den Übertragungszustand
is_transmitting = False

style.use('fivethirtyeight')

if sys.platform.startswith('win'):
    port = 'COM5'
elif sys.platform.startswith('linux') or sys.platform.startswith('darwin'):
    port = '/dev/ttyACM0'  # Beispiel für Linux/Mac, passen Sie dies an Ihren tatsächlichen Port an
else:
    raise EnvironmentError('Unsupported platform')

ser = serial.Serial(port, 9600)


fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 5))
# Listen für die Anzeige
times, graph_times, temperatures, humidities = [], [], [], []
# Listen für die Datenspeicherung (alle Einträge innerhalb einer Minute)
storage_times, storage_graph_times, storage_temperatures, storage_humidities = [], [], [], []
last_record_time = None  # Speichert den Zeitstempel des zuletzt gespeicherten Datensatzes


last_save_time = datetime.datetime.now()

def save_data():
    global storage_times, storage_temperatures, storage_humidities
    filename = datetime.datetime.now().strftime("%Y-%m-%d %H_%M_%S.csv")
    with open(filename, 'w') as file:
        file.write("Zeitstempel,Temperatur,Feuchtigkeit\n")  # Kopfzeile für die CSV-Datei
        for time, temp, humidity in zip(storage_times, storage_temperatures, storage_humidities):
            file.write(f"{time},{temp},{humidity}\n")
    print(f"Daten gespeichert in: {filename}")

def animate(i):
    global last_record_time, last_save_time, times, graph_times, temperatures, humidities, storage_times, storage_graph_times, storage_temperatures, storage_humidities
    current_time = datetime.datetime.now()

    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8', errors='ignore').rstrip()
        try:
            temperature, humidity = map(float, line.split(","))
            now_for_graph = current_time.strftime('%H:%M:%S')  # Nur Uhrzeit für die Graphenanzeige
            now_for_file = current_time.strftime('%Y-%m-%d %H:%M:%S')  # Datum und Uhrzeit für die Datei
            graph_times.append(now_for_graph)  # Fügt die Uhrzeit zur graph_times Liste hinzu
            times.append(now_for_file)  # Fügt Datum und Uhrzeit zur times Liste hinzu
            temperatures.append(temperature)
            humidities.append(humidity)

            # Überprüfen, ob diese Daten in einer neuen Sekunde aufgenommen wurden
            if last_record_time is None or last_record_time != now_for_file:
                last_record_time = now_for_file  # Aktualisieren des letzten Aufzeichnungszeitpunkts


            # Beschränken der Datenpunkte auf die letzten 60 Einträge
            graph_times = graph_times[-15:]
            times = times[-15:]
            temperatures = temperatures[-15:]
            humidities = humidities[-15:]

            # Aktualisierung der Speicherlisten (alle Datenpunkte)
            storage_graph_times.append(now_for_graph)
            storage_times.append(now_for_file)
            storage_temperatures.append(temperature)
            storage_humidities.append(humidity)

            # Aktualisieren Sie die Graphen entsprechend
            ax1.clear()
            ax1.plot(graph_times, temperatures, 'g-', label='Temperatur')
            ax1.set_title('Temperatur')
            ax1.set_xlabel('Zeit')
            ax1.set_ylabel('Temperatur (°C)')
            ax1.tick_params(axis='x', rotation=45)

            ax2.clear()
            ax2.plot(graph_times, humidities, 'b-', label='Luftfeuchtigkeit')
            ax2.set_title('Luftfeuchtigkeit')
            ax2.set_xlabel('Zeit')
            ax2.set_ylabel('Luftfeuchtigkeit (%)')
            ax2.tick_params(axis='x', rotation=45)

            plt.tight_layout()

            if (current_time - last_save_time).seconds >= 60:
                save_data()
                last_save_time = current_time
                storage_times, storage_graph_times, storage_temperatures, storage_humidities = [], [], [], []
        except ValueError:
            pass  # Ignoriert fehlerhafte Datenzeilen


ani = animation.FuncAnimation(fig, animate, interval=100)

def send_data():
    if is_transmitting:
        temp = temp_slider.get()
        humid = humid_slider.get()
        
        print(f"Sending: T{temp:.2f}H{humid:.2f}")

        ser.write(f"T{temp:.2f}H{humid:.2f}\n".encode())
        
        print(f"Temp Slider: {temp}, Humid Slider: {humid}")
        
        root.after(100, send_data)  

def toggle_transmission():
    global is_transmitting
    is_transmitting = not is_transmitting
    if is_transmitting:
        transmit_button.config(text="Stoppe Übertragung")
        send_data()  # Startet das Senden der Daten, wenn der Zustand aktiviert ist.
    else:
        transmit_button.config(text="Starte Übertragung")
        # Wenn is_transmitting False ist, stoppt das Senden automatisch.

def update_labels(event=None):
    temp_label.config(text=f"Temperatur: {temp_slider.get():.2f} °C")
    humid_label.config(text=f"Feuchtigkeit: {humid_slider.get():.2f} %")

# Tkinter GUI für die Steuerung
def create_gui():
    global temp_slider, humid_slider, temp_label, humid_label, transmit_button, root
    # Erstellt das Hauptfenster
    root = tk.Tk()
    root.title("Sensor Simulierung")

    # Schieberegler für die Temperatur
    temp_slider = ttk.Scale(root, from_=0, to=40, orient='horizontal', length=300, command=update_labels)
    temp_slider.pack(pady=40)

    # Schieberegler für die Feuchtigkeit
    humid_slider = ttk.Scale(root, from_=0, to=100, orient='horizontal', length=300, command=update_labels)
    humid_slider.pack(pady=40)

    # Labels für die Anzeige der aktuellen Werte
    temp_label = ttk.Label(root, text="Temperatur: 0 °C")
    temp_label.pack()
    humid_label =ttk.Label(root, text="Feuchtigkeit: 0 %")
    humid_label.pack()

    # Button zum Senden der Werte
    transmit_button = ttk.Button(root, text="Starte Übertragung", command=toggle_transmission)
    transmit_button.pack(pady=20)

    root.mainloop()

# Führe die GUI in einem separaten Thread aus, um gleichzeitige Ausführung zu ermöglichen
gui_thread = threading.Thread(target=create_gui, daemon=True)
gui_thread.start()

plt.show()



