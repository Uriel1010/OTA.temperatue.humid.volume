#ESP32 Water Level Monitor
This is a project for monitoring the water level in a container using an ESP32 microcontroller and several sensors. The sensors used in this project include an AHT10 temperature and humidity sensor, an HC-SR04 ultrasonic distance sensor, and the internal temperature sensor of the ESP32. The water level is calculated using the distance measured by the ultrasonic sensor and the dimensions of the container.

The project also includes an OLED display that shows the temperature, humidity, water level, and internal temperature readings, as well as a button for switching between the different readings. The ESP32 is also connected to a Wi-Fi network and sends the sensor readings to an MQTT broker.

#Setup
##Hardware
The following hardware components are used in this project:

ESP32 microcontroller
AHT10 temperature and humidity sensor
HC-SR04 ultrasonic distance sensor
OLED display
Momentary push button
Breadboard and jumper wires
Software
The following libraries are required for this project:

Adafruit_SSD1306
Adafruit_GFX
AHT10
DallasTemperature
OneWire
PubSubClient
WiFi
ArduinoOTA
You can install these libraries using the Arduino Library Manager.

Wiring
The wiring diagram for this project is shown below:

Wiring Diagram

Configuration
Before uploading the code to the ESP32, you need to set the following parameters in the code:

ssid: the SSID of your Wi-Fi network
password: the password for your Wi-Fi network
mqttServer: the hostname or IP address of your MQTT broker
mqttUser: the username for your MQTT broker (if required)
mqttPassword: the password for your MQTT broker (if required)
sensorHeight: the height of the ultrasonic sensor from the bottom of the container (in cm)
majorAxisLength: the length of the major axis of the container (in cm)
minorAxisLength: the length of the minor axis of the container (in cm)
cylinderLength: the length of the container (in cm)
Uploading the Code
To upload the code to the ESP32, follow these steps:

Connect the ESP32 to your computer using a USB cable.
Open the code in the Arduino IDE.
Select the board type and serial port from the Tools menu.
Click the Upload button.
Usage
After uploading the code to the ESP32, you can use the push button to switch between the different sensor readings on the OLED display. The ESP32 will also send the sensor readings to your MQTT broker, which you can use to monitor the water level remotely.
