#include "FastLED.h"
#include "EEPROM.h"
#define DEFAULT_NUM_LEDS 135
#define DEFAULT_MILLIAMPS 3000
#define DATA_PIN 15
#define EEPROM_SIZE 512 // Size of EEPROM in bytes
#define SERIAL_RATE 921600
#define CONFIG_PIN 13
#define ENABLE_LED_FEEDBACK false // Enable or disable LED feedback for IR receiver
#define BOOT_EFFECT_ENABLED false // Enable or disable boot effect
#define DEBUG true // Enable or disable debug messages
#define ESPAURA_VERSION "1.0.0" // Version of the ESPAURA firmware

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

// Initialise LED-array
CRGB leds[800];
unsigned long lastChaseUpdate = 0;
unsigned long chaseInterval = 20;
uint8_t mode = 0;
uint8_t lastMode = 0; // Last mode used
bool manualMode = false; // Flag to indicate if manual mode is active

struct mainConfig
{
  int LED_COUNT; // Number of LEDs
  int MILLIAMPS; // Max current in mA
};

mainConfig CONFIG;

void saveEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);  // Write 0xFF to all addresses
  }
  EEPROM.commit();
  EEPROM.write(0, 0xAA); // Write a marker to indicate EEPROM is not empty
  EEPROM.put(1, CONFIG); // Save the number of LEDs to EEPROM
  EEPROM.commit(); // Commit changes to EEPROM
  Serial.println("[EEPROM] Values saved to EEPROM");
  Serial.println("[SYSTEM] Restarting ESP to apply changes...");
  ESP.restart(); // Restart the ESP to apply changes
}

void readEEPROM() {
  //NUM_LEDS = EEPROM.read(1); // Read the number of LEDs from EEPROM
  //MILLIAMPS = EEPROM.read(2); // Read the max current from EEPROM
  EEPROM.get(1, CONFIG); // Read the number of LEDs from EEPROM
  Serial.println("[EEPROM] Values read from EEPROM");
  if (DEBUG) {
    Serial.print("[DEBUG] Taille de la structure mainConfig: ");
    Serial.println(sizeof(mainConfig));
  }
}

void initEEPROM() {
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM
  Serial.println("[EEPROM] EEPROM initialized");
  EEPROM.read(0); // Read the first byte to initialize EEPROM
  if (EEPROM.read(0) == 0xFF)
  {
    Serial.println("[EEPROM] EEPROM is empty, setting default values...");
    CONFIG.LED_COUNT = DEFAULT_NUM_LEDS; // Set default number of LEDs
    CONFIG.MILLIAMPS = DEFAULT_MILLIAMPS; // Set default max current
    saveEEPROM(); // Save default values to EEPROM
  }
  else
  {
    Serial.println("[EEPROM] EEPROM is not empty, reading values...");
    readEEPROM(); // Read values from EEPROM
  }
  Serial.println("[CONFIG] LED Count " + String(CONFIG.LED_COUNT) + " LEDs");
  Serial.println("[CONFIG] Max Current " + String(CONFIG.MILLIAMPS) + " mA");
}

void chaseEffect() {
  static uint8_t hueOffset = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastChaseUpdate >= chaseInterval) {
    lastChaseUpdate = currentMillis;

    fadeToBlackBy(leds, CONFIG.LED_COUNT, 10);
    
    for (int i = 0; i < CONFIG.LED_COUNT - 3; i++) {
      leds[i] = CHSV((hueOffset + (i * 255 / CONFIG.LED_COUNT)) % 255, 255, 255);
    }

    FastLED.show();
    hueOffset++;
  }
}

void serialHandler() {
  static String inputString = "";

  while (Serial.available()) {
    char incomingChar = Serial.read();
    if (inputString.startsWith("Ada")) {
      Serial.println("[MODE] Magic Word detected, switching to adalight !");
      Serial.print("Ada\n");
      mode = 0; // Switch to Adalight mode
      inputString = "";
      return;  // On quitte la fonction pour éviter d'exécuter d'autres commandes
    }
    if (incomingChar == '\n') {
      if (inputString.startsWith("!")) {
        if (inputString.startsWith("!SET MA, ")) {
          CONFIG.MILLIAMPS = inputString.substring(9).toInt();
          Serial.print("[CONFIG] Milliamps : ");
          Serial.println(CONFIG.MILLIAMPS);
          saveEEPROM();
          ESP.restart();
        } else if (inputString.startsWith("!SET LEDS, ")) {
          if (inputString.substring(11).toInt() > 800) {
            Serial.println("[CONFIG] Nombre de LEDs trop élevé, valeur par défaut appliquée !");
            CONFIG.LED_COUNT = DEFAULT_NUM_LEDS; // Set default number of LEDs
          } else if (inputString.substring(11).toInt() < 1) {
            Serial.println("[CONFIG] Nombre de LEDs trop faible, valeur par défaut appliquée !");
            CONFIG.LED_COUNT = DEFAULT_NUM_LEDS; // Set default number of LEDs
          } else {
            CONFIG.LED_COUNT = inputString.substring(11).toInt();
          }
          Serial.print("[CONFIG] LEDs : ");
          Serial.println(CONFIG.LED_COUNT);
          saveEEPROM();
          ESP.restart();
        } else if (inputString.startsWith("!RESET")) {
          ESP.restart();
        } else if (inputString.startsWith("!ADA")) {
          mode = 0; // Switch to Adalight mode
        } else {
          Serial.println("Commande inconnue");
        }
      }
      inputString = "";
    } else {
      inputString += incomingChar;
    }
  }
}

unsigned long serialTimeout = 100; // Temps d'attente max en ms

bool waitForSerialData(int bytesNeeded, unsigned long timeout) {
  unsigned long startTime = millis();
  while (Serial.available() < bytesNeeded) {
    if (millis() - startTime > timeout) {
      return false; // Timeout atteint
    }
    yield(); // Laisse l'ESP faire d'autres tâches (utile pour ESP8266/ESP32)
  }
  return true;
}

void adalight() {
  static char configBuffer[7] = {0}; // Buffer pour stocker "config"

  // Vérifier si "config" est tapé dans le port série
  while (Serial.available()) {
    char c = Serial.read();

    // Décalage du buffer et ajout du nouveau caractère
    for (uint8_t j = 0; j < 5; j++) {
      configBuffer[j] = configBuffer[j + 1];
    }
    configBuffer[5] = c;
    configBuffer[6] = '\0';

    // Vérifier si "config" est dans le buffer
    if (strcmp(configBuffer, "config") == 0) {
      mode = 1;
      return;
    }
  }

  for (i = 0; i < sizeof prefix; ++i) {
    waitLoop:
    if (!waitForSerialData(1, serialTimeout)) return;
    if (prefix[i] == Serial.read()) continue;
    i = 0;
    goto waitLoop;
  }

  if (!waitForSerialData(3, serialTimeout)) return;
  hi = Serial.read();
  lo = Serial.read();
  chk = Serial.read();

  if (chk != (hi ^ lo ^ 0x55)) {
    i = 0;
    goto waitLoop;
  }

  memset(leds, 0, CONFIG.LED_COUNT * sizeof(struct CRGB));

  for (uint8_t i = 0; i < CONFIG.LED_COUNT; i++) {
    if (!waitForSerialData(3, serialTimeout)) return;
    byte r = Serial.read();
    byte g = Serial.read();
    byte b = Serial.read();
    leds[i] = CRGB(r, g, b);
  }
  FastLED.show();
}


void serialCatch()
{
  // Check if data is available
  while (Serial.available() > 0)
  {
    char incomingByte = Serial.read();
    Serial.print("Received: ");
    Serial.println(incomingByte);
  }
}

void bootEffect(bool rainbow) {
  static uint8_t hueOffset = 0;
  for (int i = 0; i < CONFIG.LED_COUNT - 3; i++) {
    fadeToBlackBy(leds, CONFIG.LED_COUNT, 10);
    for (int j = 0; j < 1; j++) {
      if (rainbow) {
        leds[i + j] = CHSV((hueOffset + (i + j) * 255 / CONFIG.LED_COUNT) % 255, 255, 255);
      } else {
        leds[i + j] = CRGB::Red; // Set all LEDs to green
      }
    } // Apply brightness limiter
    FastLED.show();
    Serial.print("#");
    delay(20);
  }
  Serial.println("");
  Serial.println("[EFFECTS] Boot effect finished !");
  hueOffset++;
  for (int i = 0; i < CONFIG.LED_COUNT; i++) {
    leds[i] = CRGB::Black; // Set all LEDs to black
  }
  FastLED.show(); // Show the LEDs
}

void setup()
{
  // Use NEOPIXEL to keep true colors
  Serial.begin(SERIAL_RATE);
  Serial.println("");
  Serial.println("[Serial] Serial initialized");
  Serial.println("[ESPAURA] Thank you for using ESPAURA !");
  Serial.println("[ESPAURA] Version: " + String(ESPAURA_VERSION));
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT); // Set the built-in LED pin as output
  digitalWrite(LED_BUILTIN, HIGH); // Turn off the built-in LED
  Serial.println("[PIN] Pins initialized !");
  initEEPROM(); // Initialize EEPROM
  // Check if the EEPROM is empty and set default value if needed *

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, CONFIG.LED_COUNT);
  FastLED.setMaxRefreshRate(60); // Set max refresh rate to 60 FPS
  FastLED.setMaxPowerInVoltsAndMilliamps(5, CONFIG.MILLIAMPS); // Set max power in volts and milliamps
  Serial.println("[LED] LEDs initialized !");
  if (BOOT_EFFECT_ENABLED) {
    Serial.println("[LED] Running boot effect");
    bootEffect(true); // Boot effect with no rainbow
  }
  if (DEBUG) {
    Serial.printf("[DEBUG]Free heap: %d bytes\n", ESP.getFreeHeap());
  }
  // Send "Magic Word" string to host
  Serial.print("Ada\n");
}

void loop()
{
  if (digitalRead(CONFIG_PIN) == LOW) {
    if (mode != 1) {
      Serial.println("[Mode] Config button pushed, config mode");
      mode = 1;
    }
  }
  if (mode == 0) {
    adalight();  // Passer en mode Adalight si en mode Ambilight
  } else if (mode == 1) {
    //serialCatch(); // Si en mode config, gérer la capture série // Garder un effet si besoin
    serialHandler(); // Si en mode config, gérer la capture série
    chaseEffect(); // Garder un effet si besoin
  } else if (mode == 2) {
    // Si en mode Ambilight, gérer la capture série
    chaseEffect(); // Garder un effet si besoin
  } else if (mode == 3) {
    // Si en mode Ambilight, gérer la capture série
    chaseEffect(); // Garder un effet si besoin
  } else {
    Serial.println("[ERROR] Unknow mode, Adalight mode");
    mode = 0; // Reset to Adalight mode
  }
}