#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define the sensor pins and the checkwaterlevel pin
const uint8_t sensorPins[] = { 12, 11, 10, 9, 8, 7 }; // 6 water level sensors
const uint8_t checkwaterlevelPin = 6; // Push button for predicting water level
const uint8_t estimateTimePin = 4; // Push button for estimating time to 100% or 0%
const uint8_t pumpControlPin = 5; // Pin to control the pump (connected to a relay or similar device)
const uint8_t manualPumpPin = 3; // Push button to manually control pump

// Define arrays to track sensor states and trigger times
bool sensorStates[6] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };
unsigned long sensortriggermultipletime[6] = {0, 0, 0, 0, 0, 0};

bool predictwaterlevel = HIGH; // Track the previous state of the predict sensor
bool estimateTimeState = HIGH; // Track the previous state of the estimate time sensor
bool manualPumpState = HIGH; // Track the previous state of the manual pump sensor

// Variables to track water level and rate of change
int lastLevel = -1;
float rateOfChange = 0.0;
int currentLevel = -1;
int lastAlarmLevel = -1;
bool pumpManualState = false; // Track the manual state of the pump

// Overflow handling
unsigned long previousMillis = 0;
unsigned long overflowOffset = 0;

// Running time
unsigned long startTime = 0;

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("C2I WATER TANK");
  Serial.println("C2I WATER TANK");
  
  // Initialize sensor pins
  for (byte i = 0; i < 6; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
  }
  pinMode(checkwaterlevelPin, INPUT_PULLUP); // Initialize the checkwaterlevel pin
  pinMode(estimateTimePin, INPUT_PULLUP); // Initialize the estimate time pin
  pinMode(manualPumpPin, INPUT_PULLUP); // Initialize the manual pump control pin
  pinMode(pumpControlPin, OUTPUT); // Initialize the pump control pin
  digitalWrite(pumpControlPin, LOW); // Ensure pump is initially off

  // Initialize start time
  startTime = getCorrectedMillis();
}

void loop() {
  handleMillisOverflow();
  checksensors();
  handlePredictsensor();
  handleEstimateTimeSensor();
  handleManualPumpSensor();
  checkAlarmCondition();
  controlPump();
  displayRunningHours();
  delay(100);
}

void handleMillisOverflow() {
  unsigned long currentMillis = millis();
  if (currentMillis < previousMillis) {
    overflowOffset += 0xFFFFFFFF; // Increment the overflow offset by the max value of unsigned long
  }
  previousMillis = currentMillis;
}

unsigned long getCorrectedMillis() {
  return millis() + overflowOffset;
}

void checksensors() {
  for (byte i = 0; i < 6; i++) {
    byte sensorPin = sensorPins[i];
    bool currentState = digitalRead(sensorPin);

    // Check if the sensor state has changed from HIGH to LOW
    if (currentState == LOW && sensorStates[i] == HIGH) {
      handlesensorTrigger(i);
    }
    sensorStates[i] = currentState;
  }
}

void handlesensorTrigger(byte sensorIndex) {
  unsigned long currentTime = getCorrectedMillis();
  currentLevel = sensorIndex * 20; // Update current water level

  if (lastLevel != -1 && lastLevel != sensorIndex) {
    calculateRateOfChange(currentTime, sensorIndex);
  }

  displayWaterLevel(sensorIndex);
  sensortriggermultipletime[sensorIndex] = currentTime; // Update the last trigger time
  lastLevel = sensorIndex; // Update the last level
}

void calculateRateOfChange(unsigned long currentTime, byte sensorIndex) {
  unsigned long timeDiff = currentTime - sensortriggermultipletime[lastLevel];
  float timeDiffMinutes = timeDiff / 60000.0; // Convert to minutes
  rateOfChange = (lastLevel * 20.0 - sensorIndex * 20.0) / timeDiffMinutes; // Percentage per minute
  String rateMsg = "Rate: ";
  if (rateOfChange < 0) {
    rateMsg += String(rateOfChange, 2) + "% per min"; // Negative indicates decrease
  } else {
    rateMsg += "+" + String(rateOfChange, 2) + "% per min"; // Positive indicates increase
  }
  displayMessage(2, rateMsg);
  Serial.println(rateMsg);
}

void displayWaterLevel(byte sensorIndex) {
  String levelMsg;
  switch (sensorIndex) {
    case 0: levelMsg = "Water level: Full"; break;
    case 1: levelMsg = "Water level: 80%"; break;
    case 2: levelMsg = "Water level: 60%"; break;
    case 3: levelMsg = "Water level: 40%"; break;
    case 4: levelMsg = "Water level: 20%"; break;
    case 5: levelMsg = "Water level: 0%"; break;
  }
  displayMessage(1, levelMsg);
  Serial.println(levelMsg);
}

void handlePredictsensor() {
  bool currentPredictState = digitalRead(checkwaterlevelPin);
  if (currentPredictState == LOW && predictwaterlevel == HIGH) {
    predictWaterLevel();
  }
  predictwaterlevel = currentPredictState;
}

void predictWaterLevel() {
  if (lastLevel != -1) {
    unsigned long currentTime = getCorrectedMillis();
    unsigned long timeDiff = currentTime - sensortriggermultipletime[lastLevel];
    float timeDiffMinutes = timeDiff / 60000.0; // Convert to minutes
    float predictedLevel = 100 - (lastLevel * 20.0 - (rateOfChange * timeDiffMinutes)); // Prediction based on rate of change
    predictedLevel = constrain(predictedLevel, 0, 100); // Ensure predicted level is within 0-100%
    String predictMsg = "Predicted water level: " + String(predictedLevel) + "%";
    displayMessage(3, predictMsg);
    Serial.println(predictMsg);
  } else {
    String errorMsg = "Not enough data to make a prediction";
    displayMessage(3, errorMsg);
    Serial.println(errorMsg);
  }
}

void handleEstimateTimeSensor() {
  bool currentEstimateState = digitalRead(estimateTimePin);
  if (currentEstimateState == LOW && estimateTimeState == HIGH) {
    estimateTimeToFullOrEmpty();
  }
  estimateTimeState = currentEstimateState;
}

void estimateTimeToFullOrEmpty() {
  if (rateOfChange != 0 && lastLevel != -1) {
    float timeToTarget = 0;
    if (rateOfChange > 0) {
      timeToTarget = (100 - (lastLevel * 20.0)) / rateOfChange;
    } else {
      timeToTarget = (lastLevel * 20.0) / -rateOfChange;
    }
    float timeToTargetMinutes = timeToTarget * 60; // Convert to minutes
    unsigned long hours = (unsigned long)(timeToTargetMinutes / 60);
    unsigned long minutes = (unsigned long)(timeToTargetMinutes) % 60;
    char timeBuffer[10];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02lu:%02lu", hours, minutes);
    String estimateMsg = "Time to target: " + String(timeBuffer);
    displayMessage(3, estimateMsg);
    Serial.println(estimateMsg);
  } else {
    String errorMsg = "Rate of change not available";
    displayMessage(3, errorMsg);
    Serial.println(errorMsg);
  }
}

void handleManualPumpSensor() {
  bool currentManualPumpState = digitalRead(manualPumpPin);
  if (currentManualPumpState == LOW && manualPumpState == HIGH) {
    pumpManualState = !pumpManualState; // Toggle pump state
    digitalWrite(pumpControlPin, pumpManualState ? HIGH : LOW);
    String pumpMsg = pumpManualState ? "Pump manually ON" : "Pump manually OFF";
    displayMessage(3, pumpMsg);
    Serial.println(pumpMsg);
  }
  manualPumpState = currentManualPumpState;
}

void checkAlarmCondition() {
  if (currentLevel != -1 && (currentLevel <= 20 || currentLevel >= 80)) {
    if (lastAlarmLevel == -1 || abs(currentLevel - lastAlarmLevel) >= 20) {
      String alarmMsg = (currentLevel <= 20) ? "ALARM: Low water level!" : "ALARM: High water level!";
      displayMessage(3, alarmMsg);
      Serial.println(alarmMsg);
      lastAlarmLevel = currentLevel;
    }
  }
}

void controlPump() {
  if (!pumpManualState) { // Only auto control pump if not in manual mode
    if (currentLevel == 5) { // Water level at 0%
      digitalWrite(pumpControlPin, HIGH); // Turn on the pump
      String pumpMsg = "Pump ON: Water level at 0%";
      displayMessage(3, pumpMsg);
      Serial.println(pumpMsg);
    } else if (currentLevel == 0) { // Water level full
      digitalWrite(pumpControlPin, LOW); // Turn off the pump
      String pumpMsg = "Pump OFF: Water level full";
      displayMessage(3, pumpMsg);
      Serial.println(pumpMsg);
    }
  }
}

void displayRunningHours() {
  unsigned long currentMillis = getCorrectedMillis();
  unsigned long elapsedMillis = currentMillis - startTime;
  unsigned long hours = (elapsedMillis / 3600000) % 24;
  unsigned long minutes = (elapsedMillis / 60000) % 60;
  unsigned long seconds = (elapsedMillis / 1000) % 60;

  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  
  String runningHoursMsg = "Running: " + String(timeBuffer);
  displayMessage(4, runningHoursMsg);
  Serial.println(runningHoursMsg);
}

void displayMessage(int row, String message) {
  lcd.setCursor(0, row);
  lcd.print("                    "); // Clear the specific line
  lcd.setCursor(0, row);
  lcd.print(message);
}
