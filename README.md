# tankmonitor
This project is a water level monitoring system that uses water level sensors and an LCD to display the water level, rate of change, predicted water level, and running hours of the system. The system can also control a pump based on the water level and includes manual control features.

Hardware Requirements

  Arduino (e.g., Arduino Uno)
  6 Water level sensors
  LCD Display (20x4) with I2C interface
  Push buttons (3x)
  Relay module (to control the pump)
  Wires and breadboard
  Pin Connections

Water Level Sensors:

  Sensor 1: Pin 12
  Sensor 2: Pin 11
  Sensor 3: Pin 10
  Sensor 4: Pin 9
  Sensor 5: Pin 8
  Sensor 6: Pin 7

Push Buttons:

  Predict Water Level: Pin 6
  Estimate Time to Full/Empty: Pin 4
  Manual Pump Control: Pin 3
  Relay Module (Pump Control):

Relay Control: 
  
  Pin 5

LCD Display:

  SDA: A4
  SCL: A5

Usage

  Connect the hardware according to the pin connections described above.
  Upload the code to your Arduino.
  Monitor the Serial Output for debugging information.
  Observe the LCD Display to see the water level, rate of change, predicted water level, running hours, and any alarm messages.
  Use the push buttons to predict water level, estimate time to full/empty, and manually control the pump.

Example Display

  Line 1: "C2I WATER TANK"
  Line 2: "Rate: -0.05% per min" (example of a negative rate)
  Line 3: "Water level: 60%"
  Line 4: "Running: 01:23:45"

Note

  Ensure the push buttons are properly debounced if needed, as the code does not include debouncing logic. Test the system thoroughly to verify all functionalities.
