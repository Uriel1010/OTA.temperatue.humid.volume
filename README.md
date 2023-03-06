<!DOCTYPE html>
<html>
  <head>
    <title>ESP32 Water Level Monitor</title>
    <meta charset="UTF-8">
  </head>
  <body>
    <h1>ESP32 Water Level Monitor</h1>
    <p>This project is a water level monitoring system built with the ESP32 microcontroller and several sensors. The system measures the distance to the water surface in a container using an ultrasonic sensor, calculates the height of the water level, and estimates the volume of water in the container using an elliptical cylinder model. The system also measures the temperature and humidity using an AHT10 sensor and the internal temperature of the ESP32 microcontroller using a Dallas Temperature Sensor. The data is transmitted over Wi-Fi and can be viewed in real-time on an OLED display and an MQTT dashboard.</p>
    
    <h2>Hardware</h2>
    <ul>
      <li>ESP32 microcontroller</li>
      <li>128x64 OLED display</li>
      <li>HC-SR04 ultrasonic sensor</li>
      <li>AHT10 temperature and humidity sensor</li>
      <li>Dallas Temperature Sensor</li>
      <li>Push button</li>
    </ul>
    
    <h2>Software</h2>
    <ul>
      <li>Arduino IDE</li>
      <li>ESP32 board support package for Arduino IDE</li>
      <li>PubSubClient library for MQTT communication</li>
      <li>Adafruit_SSD1306 library for OLED display</li>
      <li>NewPing library for ultrasonic sensor</li>
      <li>AHT10 library for temperature and humidity sensor</li>
      <li>DallasTemperature library for Dallas Temperature Sensor</li>
    </ul>
    
    <h2>Installation and Setup</h2>
    <ol>
      <li>Install the Arduino IDE and the ESP32 board support package for Arduino IDE.</li>
      <li>Install the necessary libraries: PubSubClient, Adafruit_SSD1306, NewPing, AHT10, and DallasTemperature.</li>
      <li>Download or clone the project code from GitHub.</li>
      <li>Open the "water_level_monitor" sketch in the Arduino IDE.</li>
      <li>Modify the Wi-Fi and MQTT connection settings in the sketch to match your network and server settings.</li>
      <li>Upload the sketch to the ESP32 microcontroller.</li>
      <li>Connect the hardware components to the ESP32 microcontroller as per the schematic diagram.</li>
      <li>Power on the ESP32 microcontroller and check the OLED display and MQTT dashboard for real-time water level, temperature, and humidity data.</li>
    </ol>
    
    <h2>Image</h2>
    <!-- Placeholder for image -->
    
  </body>
</html>
