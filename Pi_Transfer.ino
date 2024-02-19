#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin-Zuweisungen
const int tempSensorPin = A0; // Temperatursensor
const int moistureSensorPin = A1; // Feuchtigkeitssensor
const int relayPin = 2; // Relais für Klimaanlagensteuerung
const int redLedPin = 5; // Rote LED für "zu warm"
const int yellowLedPin = 6; // Gelbe LED für "angenehm"
const int greenLedPin = 7; // Grüne LED für "zu kalt"

// Initialisieren des LCDs mit der I2C-Adresse (häufig 0x27 oder 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adresse, Zeichen pro Zeile, Anzahl der Zeilen

void setup() {
  Serial.begin(9600); // Starte die serielle Kommunikation
  lcd.init(); // LCD initialisieren
  lcd.backlight(); // Hintergrundbeleuchtung einschalten
  
  pinMode(tempSensorPin, INPUT);
  pinMode(moistureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
}

void loop() {
  static float temperatur = 0.0; // Initialisiere mit 0.0, um Startwerte zu haben
  static float humidity = 0.0;

  
  // Temperatursensor auslesen und umwandeln
  int sensorWert = analogRead(tempSensorPin);
  temperatur = map(sensorWert, 0, 410, -50, 150);

  // Feuchtigkeitssensor auslesen und umwandeln
  int moistureValue = analogRead(moistureSensorPin);
  humidity = map(moistureValue, 0, 800, 0, 100);

  // Überprüfen, ob Daten über die serielle Schnittstelle verfügbar sind
  if (Serial.available() > 0) {
    String received = Serial.readStringUntil('\n'); // Liest die empfangene Nachricht bis zum Zeilenende
    Serial.print("Received: ");
    Serial.println(received); // Bestätige den Empfang im seriellen Monitor

    int tIndex = received.indexOf('T');
    int hIndex = received.indexOf('H');
    if (tIndex != -1 && hIndex != -1) {
      String tempStr = received.substring(tIndex + 1, hIndex);
      String humidStr = received.substring(hIndex + 1);
      temperatur = tempStr.toFloat();
      humidity = humidStr.toFloat();
    }
  }

  
  // Aktualisiere LCD-Anzeige mit den zuletzt gelesenen oder empfangenen Werten
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperatur);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Feucht: ");
  lcd.print(humidity);
  lcd.print(" %");

  // Neue Entscheidungslogik unter Berücksichtigung von Temperatur und Feuchtigkeit
bool isTempInRangeForGreen = (temperatur >= 18 && temperatur <= 22);
bool isHumidityInRangeForGreen = (humidity >= 40 && humidity <= 60);
bool isTempInRangeForYellow = ((temperatur >= 16 && temperatur < 18) || (temperatur > 22 && temperatur <= 24));
bool isHumidityInRangeForYellow = ((humidity >= 30 && humidity < 40) || (humidity > 60 && humidity <= 70));

  
if ((isTempInRangeForGreen && isHumidityInRangeForGreen)) {
  // Grünes Licht
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
} else if (
    (isTempInRangeForYellow && isHumidityInRangeForYellow) ||
    (isTempInRangeForGreen && isHumidityInRangeForYellow) ||
    (isHumidityInRangeForGreen && isTempInRangeForYellow)
  ) {
  // Gelbes Licht
  digitalWrite(yellowLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
} else {
  // Rotes Licht
  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
}

// Relaissteuerung basierend auf Temperatur
if (temperatur > 25) {
  // Einschalten im Kühlmodus
  digitalWrite(relayPin, HIGH);
} else if (temperatur < 18) {
  // Einschalten im Heizmodus
  digitalWrite(relayPin, HIGH);
} else if (temperatur >= 19 && temperatur <= 21) {
  // Ausschalten
  digitalWrite(relayPin, LOW);
}

  // Innerhalb der loop() Funktion des Arduino-Codes, ersetzen Sie die serielle Ausgabe durch:
  Serial.print(temperatur);
  Serial.print(",");
  Serial.println(humidity);


  delay(1000); // Kurze Pause vor dem nächsten Durchlauf
}
