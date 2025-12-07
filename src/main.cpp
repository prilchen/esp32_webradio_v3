#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <secrets.h>

// Display Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// VS1053 Einstellungen
#include <VS1053Driver.h>
#define UNDEFINED -1

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#define VS1053_CS 5
#define VS1053_DCS 16
#define VS1053_DREQ 4
#endif

// Forward Declarations
void updateEncoder();
void connectToHost();

// Lautstärke & Encoder
const int encoderPin1 = 34;
const int encoderPin2 = 33;
const int buttonPin = 32;
int encoderValue = 70;
int lastEncoded = 0;
int lastDisplayedVolume = -1; // Hilfsvariable für Display-Update

// Objekte
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ, UNDEFINED, SPI);
WiFiClient client;

// WiFi Einstellungen
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// --- SENDERLISTE (Erweitert um WDR) ---
const char *hosts[] = {
  // Deine Favoriten
  "ic.radiomonster.fm", 
  "ic.radiomonster.fm", 
  "icecast.omroep.nl", 
  "ice6.somafm.com",
  
  // WDR Sender (Host ist bei Icecast oft spezifisch für den Stream)
  "wdr-1live-live.icecast.wdr.de",
  "wdr-wdr2-rheinland.icecast.wdr.de",
  "wdr-wdr4-live.icecast.wdr.de",
  "wdr-wdr5-live.icecast.wdr.de",
  
  // Weitere aus deiner Liste
  "streams.80s80s.de",
  "stream.radioparadise.com"
};

const char *paths[] = {
  // Deine Favoriten Pfade
  "/evergreens.ultra", 
  "/tophits.ultra", 
  "/radio6-bb-mp3", 
  "/groovesalad-128-mp3",
  
  // WDR Pfade
  "/wdr/1live/live/mp3/128/stream.mp3",
  "/wdr/wdr2/rheinland/mp3/128/stream.mp3",
  "/wdr/wdr4/live/mp3/128/stream.mp3",
  "/wdr/wdr5/live/mp3/128/stream.mp3",
  
  // Weitere Pfade
  "/80s80smp3-high",
  "/aac-320"
};

const int httpPorts[] = { 
    80, 80, 80, 80, // Alte
    80, 80, 80, 80, // WDR
    80, 80          // Rest
};

const char *descriptions[] = {
  "RadioMonster Oldie", 
  "RadioMonster Hits", 
  "Radio 6 NL", 
  "SomaFM Groove",
  
  "1LIVE",
  "WDR 2 Rheinland",
  "WDR 4",
  "WDR 5",
  
  "80s80s Radio",
  "Radio Paradise"
};

int currentHostIndex = 0;
const int numHosts = sizeof(hosts) / sizeof(hosts[0]);

// Puffer
uint8_t mp3buff[64];

void setup() {
  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPin1), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPin2), updateEncoder, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  Serial.begin(115200);

  delay(1000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Booting V3...");
  display.display();

  SPI.begin();
  player.beginOutput();
  player.setVolume(encoderValue);

  Serial.print("Connecting to SSID ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  
  // DNS Fix (Google & Cloudflare)
  WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(1, 1, 1, 1), IPAddress(8, 8, 4, 4));

  display.clearDisplay();
  display.setCursor(0, 5);
  display.println("Prilchen's WebRadio");
  display.setCursor(0, 20);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  
  delay(1000);
  connectToHost();
}

void loop() {
  // Button: Senderwechsel
  if (digitalRead(buttonPin) == LOW) {
    currentHostIndex = (currentHostIndex + 1) % numHosts;
    connectToHost();
    delay(1000); // Einfaches Entprellen
  }

  // Encoder Logik begrenzen
  encoderValue = constrain(encoderValue, 0, 100);
  player.setVolume(encoderValue);

  // --- NEU: DISPLAY UPDATE NUR BEI ÄNDERUNG ---
  if (encoderValue != lastDisplayedVolume) {
     // Wir löschen nur den unteren Bereich (Zeile 55-64), damit der Sendername stehen bleibt
     display.fillRect(0, 55, 128, 9, BLACK); 
     display.setCursor(0, 55);
     display.print("Vol: ");
     display.print(encoderValue);
     display.print("%");
     display.display();
     
     lastDisplayedVolume = encoderValue;
  }
  // --------------------------------------------

  // Auto-Reconnect bei Stream-Abbruch
  if (!client.connected()) {  
    Serial.println("Stream lost -> next sender...");
    currentHostIndex = (currentHostIndex + 1) % numHosts;
    connectToHost();
  }

  // Stream abspielen
  if (client.available() > 0) {
    uint8_t bytesread = client.read(mp3buff, 64);
    player.playChunk(mp3buff, bytesread);
  }
}

// --- NEUE INTELLIGENTE CONNECT FUNKTION ---
void connectToHost() {
  String tempHost = hosts[currentHostIndex];
  String tempPath = paths[currentHostIndex];
  int tempPort = httpPorts[currentHostIndex];
  
  Serial.println("\n-------------------------------------");
  Serial.print("Verbinde zu Sender Index: ");
  Serial.println(currentHostIndex);

  // Wir versuchen bis zu 3 Umleitungen zu folgen
  for (int i = 0; i < 3; i++) {
    client.stop(); // Alte Verbindung sicher trennen
    
    Serial.print("Versuch "); Serial.print(i + 1);
    Serial.print(": Verbinde zu "); Serial.print(tempHost);
    
    if (!client.connect(tempHost.c_str(), tempPort)) {
      Serial.println(" -> Fehlgeschlagen!");
      break; // Abbrechen, nächster Sender in loop() wird getriggert
    }
    Serial.println(" -> Verbunden!");

    // GET Request senden
    Serial.print("Requesting: "); Serial.println(tempPath);
    client.print(String("GET ") + tempPath + " HTTP/1.1\r\n" +
                 "Host: " + tempHost + "\r\n" + 
                 "User-Agent: ESP32_Radio_Player\r\n" + // Wichtig: User-Agent setzen!
                 "Connection: close\r\n\r\n");

    // Warten auf Antwort (max 2 Sekunden)
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 2000) {
        Serial.println(">>> Timeout: Server antwortet nicht!");
        client.stop();
        return;
      }
    }

    // --- Header Analyse ---
    String statusLine = client.readStringUntil('\n');
    Serial.print("Status: "); Serial.println(statusLine);

    // Fall A: Alles gut (200 OK) -> Musik startet
    if (statusLine.indexOf("200 OK") > 0 || statusLine.indexOf("ICY 200") > 0) {
      Serial.println(">>> Stream gefunden! Viel Spaß.");
      
      // Display aktualisieren (nur bei Erfolg)
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WebRadio Playing");
      display.setCursor(0, 15);
      display.println(descriptions[currentHostIndex]);
      display.setCursor(0, 35);
      display.println("Verbunden mit:");
      display.setCursor(0, 45);
      display.println(tempHost); // Zeigt den echten Server an
      display.display();
      return; // Funktion beenden, loop() übernimmt das Abspielen
    }

    // Fall B: Umleitung (301 oder 302)
    else if (statusLine.indexOf("302") > 0 || statusLine.indexOf("301") > 0) {
      Serial.println(">>> Umleitung erkannt! Suche neue Adresse...");
      
      // Header durchsuchen nach "Location:"
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.startsWith("Location: ") || line.startsWith("location: ")) {
          String newUrl = line.substring(10); // "Location: " abschneiden
          newUrl.trim();
          Serial.print("Neue URL: "); Serial.println(newUrl);

          // URL Parsen: http://neuer-server.de/pfad -> Host & Pfad trennen
          int doubleSlash = newUrl.indexOf("//");
          int firstSlash = newUrl.indexOf("/", doubleSlash + 2);

          if (firstSlash > 0) {
            tempHost = newUrl.substring(doubleSlash + 2, firstSlash);
            tempPath = newUrl.substring(firstSlash);
            // Port bleibt meist 80, außer URL sagt was anderes (hier vereinfacht)
          } else {
             Serial.println("Konnte URL nicht parsen.");
          }
          break; // Wir haben die neue Adresse, raus aus der Header-Schleife
        }
        if (line == "\r") break; // Header Ende
      }
      // Die Schleife (for int i) läuft weiter und verbindet sich im nächsten Durchlauf mit tempHost
      continue; 
    }
    
    // Fall C: Fehler (404 etc)
    else {
      Serial.println(">>> Fehler vom Server (404 o.ä.).");
      break; 
    }
  }

  // Wenn wir hier ankommen, hat es 3x nicht geklappt
  Serial.println("Konnte Stream nach Umleitungen nicht öffnen.");
  client.stop();
  // loop() wird merken !client.connected() und zum nächsten Sender schalten
}

void updateEncoder() {
  int MSB = digitalRead(encoderPin1);
  int LSB = digitalRead(encoderPin2);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

  lastEncoded = encoded;
}