#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin-Zuweisungen
const int tempSensorPin = A0;      // Temperatursensor
const int moistureSensorPin = A1;  // Feuchtigkeitssensor
const int relayPin = 2;            // Relais für Klimaanlagensteuerung
const int redLedPin = 5;           // Rote LED
const int yellowLedPin = 6;        // Gelbe LED
const int greenLedPin = 7;         // Grüne LED

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adresse, Zeichen pro Zeile, Anzahl der Zeilen

// Globale Variablen für Temperatur und Feuchtigkeit
float currentTemperature = 0.0;
float currentHumidity = 0.0;

unsigned long lastMoistureReadTime = 0;
unsigned long lastLCDUpdateTime = 0;            // Initialisierung der lastUpdate Variable
const long moistureSensorReadInterval = 1000;              // Zeitintervall für das Abtasten des Feuchtigkeitssensors
const long lcdUpdateInterval = 1000;         // Aktualisierungsintervall für das LCD in Millisekunden
bool serialDataReciveFlag = false;       // Flag, um zu markieren, dass kürzlich Daten empfangen wurden
unsigned long lastSerialDataReceivedTime = 0;  // Zeitpunkt des letzten Datenempfangs
const long serialDataTimeout = 1000;   // Zeit in Millisekunden, nach der das Flag zurückgesetzt wird
unsigned long lastSensorDataSendTime = 0;
const long sensorDataSendInterval = 1000; // Sendeintervall in Millisekunden

// Definition des Zustands-Enums vor setup()
enum LedState {
  LED_OFF,
  LED_GREEN,
  LED_YELLOW,
  LED_RED
};

LedState currentState = LED_OFF;  // Globale Zustandsvariable

void setup() {
  Serial.begin(9600);  // Starte die serielle Kommunikation
  lcd.init();          // LCD initialisieren
  lcd.backlight();     // Hintergrundbeleuchtung einschalten

  pinMode(tempSensorPin, INPUT);
  pinMode(moistureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  if (serialDataReciveFlag && (currentMillis - lastSerialDataReceivedTime > serialDataTimeout)) {
    // Wenn seit dem letzten Empfang von Daten genug Zeit vergangen ist, zurücksetzen
    serialDataReciveFlag = false;
  }

  if (Serial.available() > 0) {
    readAndProcessSerialData();
    serialDataReciveFlag = true;
    lastSerialDataReceivedTime = currentMillis;
  } else if (!serialDataReciveFlag) {
    // Nur Sensordaten lesen, wenn kürzlich keine Daten empfangen wurden
    readSensorsAndUpdate(currentMillis);
  }
  // Aktualisiere die Anzeige nur, wenn das festgelegte Intervall abgelaufen ist
  updateDisplayIfNeeded(currentMillis);
  updateRelay(currentTemperature);
  updateLedStateMachine(currentTemperature, currentHumidity, currentMillis);


  if (currentMillis - lastSensorDataSendTime >= sensorDataSendInterval) {
    sendSensorData();
    lastSensorDataSendTime = currentMillis;
  }
}

void readSensorsAndUpdate(unsigned long currentMillis) {
  currentTemperature = readTemperatureSensor();
  if (currentMillis - lastMoistureReadTime >= moistureSensorReadInterval) {
    lastMoistureReadTime = currentMillis;
    currentHumidity = readMoistureSensor();
  }
}

float readTemperatureSensor() {
  int tempValue = analogRead(tempSensorPin);
  return map(tempValue, 0, 400, -50, 150);  // Annahme der Umrechnung
}

float readMoistureSensor() {
  int moistureValue = analogRead(moistureSensorPin);
  return map(moistureValue, 0, 800, 0, 100);  // Annahme der Umrechnung
}



// Aktualisiere LCD-Anzeige mit den zuletzt gelesenen oder empfangenen Werten
void updateDisplayIfNeeded(unsigned long currentMillis) {
  if (currentMillis - lastLCDUpdateTime >= lcdUpdateInterval) {
    lastLCDUpdateTime = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(currentTemperature);
    lcd.write(byte(223)); // Gradzeichen
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Feucht: ");
    lcd.print(currentHumidity);
    lcd.print(" %");
  }
}
void sendSensorData() {
  Serial.print(currentTemperature);
  Serial.print(",");
  Serial.println(currentHumidity);
}

void updateLedStateMachine(float temperature, float humidity, unsigned long currentMillis) {
  static unsigned long lastTransitionTime = 0;
  const long transitionInterval = 1000;  // Zeitintervall zwischen Zustandsübergängen

  // Definiere die Bedingungen für die Farbbereiche
  bool isTempInRangeForGreen = (temperature >= 18 && temperature <= 22);
  bool isHumidityInRangeForGreen = (humidity >= 40 && humidity <= 60);
  bool isTempInRangeForYellow = ((temperature >= 16 && temperature < 18) || (temperature > 22 && temperature <= 24));
  bool isHumidityInRangeForYellow = ((humidity >= 30 && humidity < 40) || (humidity > 60 && humidity <= 70));

  // Bestimme den nächsten Zustand basierend auf den definierten Bedingungen
  LedState nextState = currentState;

  if (isTempInRangeForGreen && isHumidityInRangeForGreen) {
    nextState = LED_GREEN;
  } else if (
    (isTempInRangeForYellow && isHumidityInRangeForYellow) || (isTempInRangeForGreen && isHumidityInRangeForYellow) || (isHumidityInRangeForGreen && isTempInRangeForYellow)) {
    nextState = LED_YELLOW;
  } else {
    nextState = LED_RED;
  }

  // Prüfe, ob ein Zustandswechsel erfolgen soll und ob genug Zeit seit dem letzten Wechsel vergangen ist
  if (nextState != currentState && currentMillis - lastTransitionTime > transitionInterval) {
    lastTransitionTime = currentMillis;
    currentState = nextState;

    // Aktualisiere die LEDs entsprechend dem neuen Zustand
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, LOW);

switch (currentState) {
  case LED_GREEN:
    digitalWrite(greenLedPin, HIGH);
    break;
  case LED_YELLOW:
    digitalWrite(yellowLedPin, HIGH);
    break;
  case LED_RED:
    digitalWrite(redLedPin, HIGH);
    break;
  case LED_OFF:
    // Keine Aktion erforderlich für LED_OFF
    break;
    }
  }
}

void updateRelay(float temperature) {
  static bool relayState = false;  // Behält den Zustand zwischen Aufrufen bei
  
  const float temperatureThresholdHigh = 22.0;  // Obere Temperaturgrenze
  const float temperatureThresholdLow = 18.0;   // Untere Temperaturgrenze
  const float hysteresis = 1.0;                 // Hysterese

  // Bestimmen, ob eine Zustandsänderung erforderlich ist
  bool needsToTurnOn = temperature > temperatureThresholdHigh + hysteresis || temperature < temperatureThresholdLow - hysteresis;
  bool needsToTurnOff = temperature >= temperatureThresholdLow + hysteresis && temperature <= temperatureThresholdHigh - hysteresis;

  if (!needsToTurnOn && !needsToTurnOff) return; // Keine Aktion erforderlich

  static unsigned long lastRelayToggleTime = 0;  // Zeitpunkt des letzten Zustandswechsels
  const long relayToggleInterval = 1000;         // Mindestintervall zwischen Zustandsänderungen
  unsigned long currentMillis = millis();

  // Überprüfen, ob seit der letzten Änderung genügend Zeit vergangen ist
  bool isTimeForChange = currentMillis - lastRelayToggleTime > relayToggleInterval;

  if (isTimeForChange) {
    if (needsToTurnOn && !relayState) {
      relayState = true;  // Einschalten, wenn ausgeschaltet und Bedingungen erfüllt
      lastRelayToggleTime = currentMillis;
      digitalWrite(relayPin, HIGH);
    } else if (needsToTurnOff && relayState) {
      relayState = false;  // Ausschalten, wenn eingeschaltet und Bedingungen erfüllt
      lastRelayToggleTime = currentMillis;
      digitalWrite(relayPin, LOW);
    }
  }
}

bool readAndProcessSerialData() {
  if (Serial.available() > 0) {
    String received = Serial.readStringUntil('\n');
    int tIndex = received.indexOf('T');
    int hIndex = received.indexOf('H');
    if (tIndex != -1 && hIndex != -1) {
      String tempStr = received.substring(tIndex + 1, hIndex);
      String humidStr = received.substring(hIndex + 1);
      currentTemperature = tempStr.toFloat();
      currentHumidity = humidStr.toFloat();
      return true;  // Daten wurden empfangen und verarbeitet
    }
    return false;  // Bedingungen nicht erfüllt, keine Daten verarbeitet
  }
  return false; // Keine Daten verfügbar
}
