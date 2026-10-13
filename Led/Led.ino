#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// --- KONFIGURASI WI-FI ---
const char* ssid = "Lantai_3";     // <-- Ganti dengan nama Wi-Fi rumah kamu
const char* password = "1253920464828"; // <-- Ganti dengan password Wi-Fi kamu

// Membuat objek server pada port standar HTTP (80)
ESP8266WebServer server(80);

// Menggunakan LED bawaan modul ESP8266
const int ledPin = LED_BUILTIN; 

void handleRoot() {
  server.send(200, "text/plain", "ESP8266 Web Server Aktif!");
}

void handleLEDOn() {
  digitalWrite(ledPin, LOW); // LOW justru MENYALAKAN Built-in LED (Active LOW)
  server.send(200, "text/plain", "LED_NYALA");
}

void handleLEDOff() {
  digitalWrite(ledPin, HIGH);  // HIGH justru MEMATIKAN Built-in LED
  server.send(200, "text/plain", "LED_MATI");
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi LED Bawaan
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Matikan LED bawaan di awal (HIGH = Mati)

  // Mulai menyambungkan ke Wi-Fi
  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Koneksi Berhasil, Tampilkan IP Address di Serial Monitor
  Serial.println("");
  Serial.println("Wi-Fi Terhubung!");
  Serial.print("Alamat IP ESP8266: ");
  Serial.println(WiFi.localIP());

  // Mengatur Routing URL Web Server
  server.on("/", handleRoot);
  server.on("/led/on", handleLEDOn);
  server.on("/led/off", handleLEDOff);

  // Mulai Web Server
  server.begin();
  Serial.println("HTTP Web Server telah berjalan.");
}

void loop() {
  server.handleClient(); // Menangani request yang masuk terus-menerus
}