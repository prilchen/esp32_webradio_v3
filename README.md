# ESP32 Webradio mit VS1053 und OLED

Hier nun auch die Visual Studio Code Version des Webradio. Entwickelt mit PlatformIO um ein internetbasiertes Radio mit ESP32, VS1053 Audio-Decoder und OLED-Display. 

![image](https://github.com/user-attachments/assets/7511fe00-e7cc-4308-b2ec-f783104a672f)

## 📻 Features

- **Webradio-Streaming** über WiFi
- **OLED-Display** (128x64 SSD1306) zur Anzeige von Senderinformationen
- **Rotary Encoder** zur Senderauswahl
- **VS1053 Audio-Decoder** für hohe Audioqualität
- **Einfache Senderverwaltung** im Code
- **Stabile WiFi-Verbindung** mit DNS-Unterstützung

## 🛠️ Hardware

### Benötigte Komponenten

- [ESP32 Development Board](https://amzn.to/3BlTHlF)
- [VS1053 Audio Decoder Modul](https://amzn.to/42gwzQW)
- [SSD1306 OLED Display (128x64, I2C)](https://amzn.to/4eLUNWY)
- [Rotary Encoder (KY-040)](https://amzn.to/3zmtwKy)
- **Lautsprecher** oder Kopfhörer (3,5mm Klinke)
- **Stromversorgung** (5V, min. 1A)

### Pin-Belegung

#### VS1053
```
VS1053 Pin  →  ESP32 Pin
CS          →  GPIO 5
DCS         →  GPIO 16
DREQ        →  GPIO 4
MOSI        →  GPIO 23
MISO        →  GPIO 19
SCK         →  GPIO 18
RESET       →  GPIO 17
VCC         →  5V
GND         →  GND
```

#### OLED Display (I2C)
```
OLED Pin    →  ESP32 Pin
SDA         →  GPIO 21
SCL         →  GPIO 22
VCC         →  3.3V
GND         →  GND
```

#### Rotary Encoder
```
Encoder Pin →  ESP32 Pin
CLK (A)     →  GPIO 32
DT (B)      →  GPIO 33
SW (Button) →  GPIO 25
VCC         →  3.3V
GND         →  GND
```

## 📦 Installation

### Voraussetzungen

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)

### Schritt 1: Repository klonen

1. Erstelle einen Projekt-Ordner
2. Öffne Terminal/Powershell und navigiere in den gewünschten Ordner.
3. Führe aus:
```bash
git clone https://github.com/prilchen/esp32_webradio_v3.git
cd ESP32-Webradio-PlatformIO
```

### Schritt 2: Projekt in VS Code öffnen

1. VS Code öffnen
2. `File` → `Open Folder`
3. Projekt-Ordner auswählen
4. PlatformIO lädt automatisch alle Dependencies herunter

### Schritt 3: WiFi-Credentials konfigurieren

1. Kopiere `src/secrets.h.example` zu `src/secrets.h`
   ```bash
   cp src/secrets.h.example src/secrets.h
   ```

2. Öffne `src/secrets.h` und trage deine Daten ein:
   ```cpp
   const char* ssid = "DEIN-WIFI-NAME";
   const char* password = "DEIN-PASSWORT";
   ```

**Wichtig:** Die Datei `secrets.h` wird nicht ins Repository hochgeladen (.gitignore)!

### Schritt 4: Hochladen

1. ESP32 per USB anschließen
2. In VS Code: PlatformIO-Icon → `Upload`
3. Oder Tastenkombination: `Ctrl+Alt+U`

### Schritt 5: Serial Monitor öffnen

```
PlatformIO → Monitor
```
oder `Ctrl+Alt+S`

## 🎵 Radiosender hinzufügen/ändern

Öffne `src/main.cpp` und bearbeite das Sender-Array:

```cpp
const char* radioStations[][2] = {
  {"Radio Monster", "ic.radiomonster.fm"},
  {"1Live", "wdr-1live-live.icecastssl.wdr.de"},
  {"ByteFM", "www.byte.fm:8000/stream"},
  {"Dein Sender", "stream-url-hier"}
};
```

Format: `{"Anzeigename", "Stream-URL"}`

## 🔧 Konfiguration

### platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/pschatzmann/arduino-vs1053.git
    adafruit/Adafruit GFX Library
    adafruit/Adafruit SSD1306
```

### DNS-Server

Das Projekt verwendet Google DNS (8.8.8.8) für stabile Verbindungen. Falls nötig, kann dies in `setup()` geändert werden:

```cpp
IPAddress dns1(8, 8, 8, 8);      // Google DNS
IPAddress dns2(1, 1, 1, 1);      // Cloudflare DNS
WiFi.setDNS(dns1, dns2);
```

## 🐛 Troubleshooting

### WiFi verbindet nicht
- Prüfe SSID und Passwort in `secrets.h`
- Stelle sicher, dass das 2.4 GHz-Band aktiv ist (ESP32 unterstützt kein 5 GHz)

### Kein Ton
- Prüfe Pin-Verbindungen zum VS1053
- Teste mit einem anderen Audio-Stream
- Überprüfe die Lautstärke (kann im Code angepasst werden)

### Display zeigt nichts
- Prüfe I2C-Adresse (Standard: 0x3C)
- Teste mit I2C-Scanner
- Überprüfe SDA/SCL-Verbindungen

### Compilation Errors
```bash
# Clean Build
pio run --target clean
pio run
```

## 📚 Verwendete Libraries

- [arduino-vs1053](https://github.com/pschatzmann/arduino-vs1053) - VS1053 Audio Decoder
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) - Grafik-Bibliothek
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) - OLED-Display
- Arduino ESP32 Core

## 🙏 Credits

Basierend auf dem Tutorial von **Prilchen**:
- [ESP32-Projekt: Webradio – zweiter Teil](https://prilchen-laps.de/)
- [Original Arduino IDE Version](https://github.com/prilchen/ESP32-Webradio-VS1053-OLED-Stationswahl)

Portiert zu PlatformIO für bessere Entwicklererfahrung und moderne Toolchain.

## 📝 Lizenz

Dieses Projekt steht unter der MIT-Lizenz - siehe [LICENSE](LICENSE) Datei für Details.

## 🤝 Beitragen

Contributions sind willkommen! 

1. Fork das Repository
2. Erstelle einen Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit deine Änderungen (`git commit -m 'Add some AmazingFeature'`)
4. Push zum Branch (`git push origin feature/AmazingFeature`)
5. Öffne einen Pull Request

## 📧 Kontakt

Bei Fragen oder Problemen, öffne ein [Issue](https://github.com/prilchen/ESP32-Webradio-PlatformIO/issues).

---

⭐ **Gefällt dir das Projekt? Gib ihm einen Stern!** ⭐
