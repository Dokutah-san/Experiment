#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h> 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String ssid = "Lantai_3";
String password = "1253920464828;

ESP8266WebServer server(80);

const int LED_PIN = LED_BUILTIN;

long dailyCooldownEnd = 0;
long voteCooldownEnd = 0;
long dropCooldownEnd = 0;
long grabCooldownEnd = 0;

unsigned long lastBlinkMillis = 0;
const long blinkInterval = 500;
bool ledState = HIGH;

// --- KONFIGURASI TOMBOL KEYPAD SESUAI DATA TOKOPEDIA ---
const byte ROWS = 4; 
const byte COLS = 4; 

// Pemetaan karakter disesuaikan dengan urutan scan hardware matrix pembacaan
char hexaKeys[ROWS][COLS] = {
  {'D','*','0','#'},
  {'C','7','8','9'},
  {'B','4','5','6'},
  {'A','1','2','3'}
};

// Urutan Pin Baru: Bebas dari TX/RX agar Serial Monitor tidak memunculkan tanda tanya (?)
byte rowPins[ROWS] = {16, 0, 2, 15}; // Kabel 1 sampai 4 terhubung ke: D0, D3, D4, D8
byte colPins[COLS] = {4, 5, 12, 14}; // Kabel 8 sampai 5 terhubung ke: D2, D1, D6, D5

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

bool inputMode = false;
bool inputSsidMode = false;
String tempInput = "";

void handleRoot() {
  server.send(200, "text/plain", "ESP8266 Karuta Tracker Ready");
}

void handleSyncKrm() {
  long currentMillis = millis();
  if (server.hasArg("daily")) {
    long minLeft = server.arg("daily").toInt();
    dailyCooldownEnd = (minLeft == 0) ? 0 : currentMillis + (minLeft * 60000);
  }
  if (server.hasArg("vote")) {
    long minLeft = server.arg("vote").toInt();
    voteCooldownEnd = (minLeft == 0) ? 0 : currentMillis + (minLeft * 60000);
  }
  if (server.hasArg("drop")) {
    long minLeft = server.arg("drop").toInt();
    dropCooldownEnd = (minLeft == 0) ? 0 : currentMillis + (minLeft * 60000);
  }
  if (server.hasArg("grab")) {
    long minLeft = server.arg("grab").toInt();
    grabCooldownEnd = (minLeft == 0) ? 0 : currentMillis + (minLeft * 60000);
  }
  server.send(200, "text/plain", "Sync Success");
}

void handleDailyCooldown() { dailyCooldownEnd = millis() + 86400000; server.send(200, "text/plain", "Daily Started"); }
void handleVoteCooldown()  { voteCooldownEnd = millis() + 43200000;  server.send(200, "text/plain", "Vote Started"); }
void handleDropCooldown()  { dropCooldownEnd = millis() + 1800000;   server.send(200, "text/plain", "Drop Started"); }
void handleGrabCooldown()  { grabCooldownEnd = millis() + 600000;    server.send(200, "text/plain", "Grab Started"); }

void formatAndPrintTime(long cooldownEnd, long currentMillis, int x, int y) {
  display.setTextSize(1);
  display.setCursor(x, y);
  if (cooldownEnd > currentMillis) {
    long remainingSec = (cooldownEnd - currentMillis) / 1000;
    long hours = remainingSec / 3600;
    if (hours >= 1) {
      long minutes = (remainingSec % 3600) / 60;
      if(hours < 10) display.print("0");
      display.print(hours); display.print("h ");
      if(minutes < 10) display.print("0");
      display.print(minutes); display.print("m");
    } else {
      long minutes = remainingSec / 60;
      long seconds = remainingSec % 60;
      if(minutes < 10) display.print("0");
      display.print(minutes); display.print(":");
      if(seconds < 10) display.print("0");
      display.print(seconds);
    }
  } else {
    display.print("READY");
  }
}

void checkLedStatus(long currentMillis) {
  bool dailyReady = (dailyCooldownEnd <= currentMillis);
  bool voteReady  = (voteCooldownEnd <= currentMillis);
  bool dropReady  = (dropCooldownEnd <= currentMillis);
  bool grabReady  = (grabCooldownEnd <= currentMillis);

  if (dropReady && !grabReady && !dailyReady && !voteReady) {
    ledState = HIGH;
    digitalWrite(LED_PIN, ledState);
  }
  else if (dailyReady || voteReady || dropReady || grabReady) {
    if (currentMillis - lastBlinkMillis >= blinkInterval) {
      lastBlinkMillis = currentMillis;
      ledState = !ledState; 
      digitalWrite(LED_PIN, ledState);
    }
  } 
  else {
    ledState = HIGH;
    digitalWrite(LED_PIN, ledState); 
  }
}

void connectToWiFi() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);  display.println("Connecting WiFi...");
  display.setCursor(0, 30);  display.print("SSID: "); display.println(ssid);
  display.display();

  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter < 20) { 
    delay(500); 
    timeoutCounter++;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  long currentMillis = millis();

  if (inputMode) {
    display.setTextSize(1);
    display.setCursor(0, 2);
    if (inputSsidMode) {
      display.println("--- SETUP SSID ---");
    } else {
      display.println("--- SETUP PASSWORD ---");
    }
    display.setCursor(0, 20);
    display.print("Input: "); display.println(tempInput);
    display.setCursor(0, 50);
    display.println("*:Clear  #:Next/Save");
    display.display();
    return;
  }

  display.setCursor(0, 2);  display.print("Daily : "); formatAndPrintTime(dailyCooldownEnd, currentMillis, 54, 2);
  display.setCursor(0, 18); display.print("Vote  : "); formatAndPrintTime(voteCooldownEnd, currentMillis, 54, 18);
  display.setCursor(0, 34); display.print("Drop  : "); formatAndPrintTime(dropCooldownEnd, currentMillis, 54, 34);
  display.setCursor(0, 50); display.print("Grab  : "); formatAndPrintTime(grabCooldownEnd, currentMillis, 54, 50);

  display.display();
  checkLedStatus(currentMillis);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); 

  Serial.begin(115200);
  Serial.println("\n--- System Started ---");
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  
  connectToWiFi();

  server.on("/", handleRoot);
  server.on("/sync_krm", handleSyncKrm);
  server.on("/daily_cooldown", handleDailyCooldown);
  server.on("/vote_cooldown", handleVoteCooldown);
  server.on("/drop_cooldown", handleDropCooldown);
  server.on("/grab_cooldown", handleGrabCooldown);
  
  server.begin();
}

void loop() {
  server.handleClient();
  
  char key = customKeypad.getKey();
  if (key) {
    Serial.print("Key Detected: "); Serial.println(key); // Cek ketukan tombol di Serial Monitor
    
    if (key == 'A' && !inputMode) { 
      inputMode = true;
      inputSsidMode = true;
      tempInput = "";
    } 
    else if (inputMode) {
      if (key == '#') { 
        if (inputSsidMode) {
          if (tempInput != "") ssid = tempInput;
          inputSsidMode = false;
          tempInput = ""; 
        } else {
          if (tempInput != "") password = tempInput;
          inputMode = false; 
          connectToWiFi(); 
        }
      } 
      else if (key == '*') {
        tempInput = ""; 
      } 
      else {
        if (tempInput.length() < 32) {
          tempInput += key; 
        }
      }
    }
  }
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 100) {  
    lastUpdate = millis();
    updateDisplay();
  }
}