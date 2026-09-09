import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import matplotlib.gridspec as gridspec

# --- KONFIGURACJA ---
PORT = 'COM8'      # Zmień na swój port UART
BAUD_RATE = 115200   # Zmień na prędkość swojego mikrokontrolera
MAX_POINTS = 100   # Ile punktów wykresu przechowujemy w pamięci
MAX_TERMINAL_LINES = 8 # Ile linii tekstu ma widać w terminalu na dole
# --------------------

# Inicjalizacja portu
ser = serial.Serial(PORT, BAUD_RATE, timeout=0.01)

# Bufory na dane do wykresów
x_data = deque(maxlen=MAX_POINTS)
y_wartosc = deque(maxlen=MAX_POINTS)
y_szum = deque(maxlen=MAX_POINTS)
y_prog = deque(maxlen=MAX_POINTS)
y_fastAvg = deque(maxlen=MAX_POINTS)
y_slowAvg = deque(maxlen=MAX_POINTS)
y_stan = deque(maxlen=MAX_POINTS)
y_trend = deque(maxlen=MAX_POINTS)

# Bufor na tekst do terminala
terminal_lines = deque(maxlen=MAX_TERMINAL_LINES)
x_counter = 0

# --- KONFIGURACJA WYGLĄDU OKNA ---
fig = plt.figure(figsize=(10, 9))
gs = gridspec.GridSpec(4, 1, height_ratios=[2, 2, 2, 1.5]) # Ostatnie okno na terminal

# Wykres 1
ax1 = fig.add_subplot(gs[0])
line_wartosc, = ax1.plot([], [], label='Wartość', color='blue')
line_szum, = ax1.plot([], [], label='Szum', color='gray')
line_prog, = ax1.plot([], [], label='Próg', color='red')
ax1.set_ylim(0, 350)
ax1.legend(loc='upper right', fontsize='small')
ax1.grid(True, linestyle=':', alpha=0.6)

# Wykres 2
ax2 = fig.add_subplot(gs[1], sharex=ax1)
line_fastAvg, = ax2.plot([], [], label='fastAvg', color='orange')
line_slowAvg, = ax2.plot([], [], label='slowAvg', color='green')
ax2.set_ylim(0, 500)
ax2.legend(loc='upper right', fontsize='small')
ax2.grid(True, linestyle=':', alpha=0.6)

# Wykres 3
ax3 = fig.add_subplot(gs[2], sharex=ax1)
line_stan, = ax3.plot([], [], label='stan', color='purple', drawstyle='steps-post')
line_trend, = ax3.plot([], [], label='trend', color='cyan', drawstyle='steps-post')
ax3.set_ylim(-0.2, 2.2) 
ax3.legend(loc='upper right', fontsize='small')
ax3.grid(True, linestyle=':', alpha=0.6)

# Terminal na dole
ax4 = fig.add_subplot(gs[3])
ax4.axis('off') 
terminal_bg = ax4.axhspan(0, 1, color='black') 
text_box = ax4.text(0.01, 0.9, '', transform=ax4.transAxes, verticalalignment='top', 
                    family='monospace', color='lime', fontsize=9)

def update(frame):
    global x_counter
    has_new_data = False
    
    while ser.in_waiting > 0:
        try:
            line_data = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if not line_data:
                continue
                
            terminal_lines.append(line_data)
            has_new_data = True
            
            # Parsowanie linii z formatu: "Zmienna = wartosc | Zmienna2 = wartosc2"
            parsed = {}
            parts = line_data.split('|')
            for part in parts:
                if '=' in part:
                    k, v = part.split('=', 1)
                    parsed[k.strip()] = v.strip()
            
            # Zapisywanie wyciągniętych wartości numerycznych
            if 'Wartosc' in parsed and 'fastAvg' in parsed and 'stan' in parsed:
                y_wartosc.append(float(parsed['Wartosc']))
                y_szum.append(float(parsed['Szum']))
                y_prog.append(float(parsed['Prog']))
                
                y_fastAvg.append(float(parsed['fastAvg']))
                y_slowAvg.append(float(parsed['slowAvg']))
                
                y_stan.append(float(parsed['stan']))
                y_trend.append(float(parsed['trend']))
                
                x_data.append(x_counter)
                x_counter += 1
                
        except Exception:
            pass

    if has_new_data and x_data:
        line_wartosc.set_data(x_data, y_wartosc)
        line_szum.set_data(x_data, y_szum)
        line_prog.set_data(x_data, y_prog)
        
        line_fastAvg.set_data(x_data, y_fastAvg)
        line_slowAvg.set_data(x_data, y_slowAvg)
        
        line_stan.set_data(x_data, y_stan)
        line_trend.set_data(x_data, y_trend)
        
        if x_counter > MAX_POINTS:
            ax1.set_xlim(x_counter - MAX_POINTS, x_counter)
        else:
            ax1.set_xlim(0, MAX_POINTS)
            
        text_box.set_text('\n'.join(terminal_lines))
        
    return line_wartosc, line_szum, line_prog, line_fastAvg, line_slowAvg, line_stan, line_trend, text_box

plt.tight_layout()
ani = animation.FuncAnimation(fig, update, interval=20, blit=False, cache_frame_data=False)

plt.show()
ser.close()