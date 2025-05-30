/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the
 * words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */

#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("ESP32 Keyboard", "ESP32 Inc.", 100);
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
}

void loop() {
  if (bleKeyboard.isConnected()) {
    while (Serial.available() < 2) {
    }
    char mode = Serial.read();
    char key = Serial.read();
    Serial.print("Mode: ");
    Serial.println(mode);
    Serial.print("Key: ");
    Serial.println(key);
    Serial.println(mode == '1');
    if (mode == '1') {
      bleKeyboard.press(key);
    } else {
      bleKeyboard.release(key);
    }

  } else {
    Serial.println("Waiting 1 second for connection...");
    delay(1000);
  }
}
