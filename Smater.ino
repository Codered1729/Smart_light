#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include "secrets.h"

// ------------------- USER SETTINGS -------------------
#define RELAY_PIN 2  // GPIO2 - your relay input



// ------------------- SYSTEM VARIABLES -------------------
unsigned long lastReconnectAttempt = 0;
bool relayState = false;

// ------------------- STATUS LEDs (Optional) -------------------
#define STATUS_LED  4  // optional status LED (or remove)

// ------------------- CALLBACK: CLOUD COMMAND -------------------
bool onPowerState(const String &deviceId, bool state) {
  Serial.printf("[CMD] Device: %s | State: %s\n", deviceId.c_str(), state ? "ON" : "OFF");

  relayState = state;
  digitalWrite(RELAY_PIN, relayState ? LOW : HIGH); // LOW-trigger relay

  Serial.printf("[ACTION] Relay -> %s\n", relayState ? "ON" : "OFF");
  return true;
}

// ------------------- WIFI DIAGNOSTICS -------------------
void connectWiFi() {
  Serial.println("\n[WiFi] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempt = 0;
  while(WiFi.status() != WL_CONNECTED && attempt < 20){
    Serial.print(".");
    delay(500);
    attempt++;
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(STATUS_LED, HIGH);
  } 
  else {
    Serial.println("\n[WiFi] Failed to connect!");
    digitalWrite(STATUS_LED, LOW);
  }
}

// ------------------- SINRIC DIAGNOSTICS -------------------
void startSinric() {
  Serial.println("[Cloud] Initializing SinricPro...");

  SinricProSwitch& mySwitch = SinricPro[DEVICE_ID];
  mySwitch.onPowerState(onPowerState);

  SinricPro.onConnected([](){
    Serial.println("[Cloud] Connected to SinricPro!");
  });
  SinricPro.onDisconnected([](){
    Serial.println("[Cloud] DISCONNECTED from SinricPro! Retrying...");
  });

  SinricPro.begin(APP_KEY, APP_SECRET);

  Serial.println("[Cloud] SinricPro Started");
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n================= SYSTEM BOOT =================");
  Serial.println("Device: ESP32 Relay + SinricPro");
  Serial.println("Version: Debug Enhanced");
  Serial.println("================================================\n");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  relayState = false;
  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(STATUS_LED, LOW);

  connectWiFi();
  startSinric();
}

// ------------------- WIFI + CLOUD WATCHDOG -------------------
void watchdog() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Watchdog] WiFi Lost! Reconnecting...");
    digitalWrite(STATUS_LED, LOW);
    connectWiFi();
  }

  if (!SinricPro.isConnected()) {
    Serial.println("[Watchdog] SinricPro Reconnecting...");
    SinricPro.begin(APP_KEY, APP_SECRET);
  }
}

// ------------------- MAIN LOOP -------------------
void loop() {
  SinricPro.handle();
  watchdog();
}
