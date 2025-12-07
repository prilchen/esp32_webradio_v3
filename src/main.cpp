#include <Arduino.h>  // <- Wichtig in PlatformIO!
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <secrets.h>
//#include <VS1053.h>


#define SCREEN_WIDTH 128  // OLED display width
#define SCREEN_HEIGHT 64  // OLED display height
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include <VS1053Driver.h>
#define UNDEFINED -1

#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#define VS1053_CS D1
#define VS1053_DCS D0
#define VS1053_DREQ D3
#endif

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#define VS1053_CS 5
#define VS1053_DCS 16
#define VS1053_DREQ 4
#endif

// Forward Declarations
void updateEncoder();
void connectToHost();

// Lautstärke:
const int encoderPin1 = 34;
const int encoderPin2 = 33;
const int buttonPin = 32;
int encoderValue = 70;
int lastEncoded = 0;
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ, UNDEFINED, SPI);
WiFiClient client;

// WiFi Einstellungen
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;


// Liste der Radiostationen, Pfade und Beschreibungen als Array
const char *hosts[] = {
  "ic.radiomonster.fm", "ic.radiomonster.fm", "ic.radiomonster.fm", "ic.radiomonster.fm",
  "icecast.omroep.nl", "puma.streemlion.com",
  "stream.radioparadise.com", "icecast.omroep.nl", "jazzradio.ice.infomaniak.ch", "stream.klassikradio.de",
  "ice6.somafm.com", "top40.radio.net", "st02.sslstream.dlf.de", "streams.80s80s.de",
  "icecast.omroep.nl", "ice6.somafm.com"
};

const char *paths[] = {
  "/evergreens.ultra", "/tophits.ultra", "/dance.ultra", "/schlager.ultra",
  "/radio6-bb-mp3", "/stream",
  "/aac-320", "/radio6-bb-mp3", "/jazzradio-high.mp3", "/klassikfm-high",
  "/groovesalad-128-mp3", "/top40-mp3", "/dlf/02/mid.mp3", "/80s80smp3-high",
  "/radio2-bb-mp3", "/secretagent-128-mp3"
};

const int httpPorts[] = { 80, 80, 80, 80, 80, 1960, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80 };

// Beschreibungen der Sender für das Display
const char *descriptions[] = {
  "Evergreens", "Top Hits", "Dance Hits", "Schlager",
  "Radio 6 NL", "Puma Stream",
  "Radio Paradise", "Chill Out Zone", "Jazz Radio", "Klassik Radio",
  "Groove Salad", "Top Hits by Radio.net", "DLF Kultur", "80s80s Radio",
  "The Rock!", "SomaFM Secret Agent"
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
  Serial.begin(115200);

  delay(3000);
  display.clearDisplay();

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
// DNS-Server explizit setzen - HIER EINFÜGEN!
  WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(1, 1, 1, 1), IPAddress(8, 8, 4, 4));
  
  // Oder alternativ nur DNS ohne andere Einstellungen zu ändern:
  // IPAddress dns1(8, 8, 8, 8);      // Google DNS
  // IPAddress dns2(1, 1, 1, 1);      // Cloudflare DNS
  // WiFi.setDNS(dns1, dns2);

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  display.println("Prilchen`s WebRadio");
  display.setCursor(0, 20);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.setCursor(0, 30);
  display.print("Internet gefunden");
  display.display();
  connectToHost();
  delay(3000);
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    currentHostIndex = (currentHostIndex + 1) % numHosts;
    connectToHost();
    delay(1000);
  }

  encoderValue = constrain(encoderValue, 0, 100);
  player.setVolume(encoderValue);

  //wenn stream defekt dann nächster sender
  if (!client.connected()) {  
    Serial.println("next sender...");
    currentHostIndex = (currentHostIndex + 1) % numHosts;  // Nächsten Sender versuchen
    connectToHost();                                       // Versuche erneut, den nächsten Sender zu verbinden
  }

  if (client.available() > 0) {
    uint8_t bytesread = client.read(mp3buff, 64);
    player.playChunk(mp3buff, bytesread);
  }
}

void connectToHost() {
  client.stop();
  const char *host = hosts[currentHostIndex];
  const char *path = paths[currentHostIndex];
  int httpPort = httpPorts[currentHostIndex];

  Serial.print("connecting to ");
  Serial.println(host);

  //wenn stream defekt dann nächster sender
  if (!client.connect(host, httpPort)) {  
    Serial.println("Connection failed");
    currentHostIndex = (currentHostIndex + 1) % numHosts;  // Nächsten Sender versuchen
    connectToHost();                                       // Versuche erneut, den nächsten Sender zu verbinden
    return;
  }

  Serial.print("Requesting stream: ");
  Serial.println(path);
  client.print(String("GET ") + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  display.clearDisplay();
  display.setCursor(10, 5);
  display.println("WebRadioStation:");
  display.setCursor(0, 20);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.setCursor(0, 35);
  display.println("Sender:");
  display.setCursor(0, 45);
  display.println(descriptions[currentHostIndex]);
  display.display();
}

void updateEncoder() {
  int MSB = digitalRead(encoderPin1);      // MSB = Bit mit dem höchsten Stellenwert
  int LSB = digitalRead(encoderPin2);      // LSB = Bit mit dem niedrigsten Stellenwert
  int encoded = (MSB << 1) | LSB;          // Umwandlung des 2-Pin-Wertes in eine einzelne Zahl
  int sum = (lastEncoded << 2) | encoded;  // Hinzufügung zum vorherigen kodierten Wert

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

  lastEncoded = encoded;  // diesen Wert für das nächste Mal speichern
}