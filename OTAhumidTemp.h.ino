/*
Project Name: Uriel Manzur's AHT10 Temperature and Humidity Monitoring System
Project Overview: This project is a temperature and humidity monitoring system
that uses an AHT10 sensor and an ESP32 microcontroller to collect data and transmit it to an MQTT server.
The system is designed to take readings every five minutes and transmit the data to the server using the MQTT protocol.
Additionally, the system has the option for OTA (Over-the-Air) updates, allowing for remote firmware updates to the ESP32.

Hardware Requirements:

ESP32 microcontroller
AHT10 temperature and humidity sensor
Breadboard or custom PCB
Jumper wires
3.3V power supply
Ultrasonic sensor for volume measurement
Push button for switching between display screens
Wiring Diagram:

AHT10 sensor:
Vin => 3V
GND => GND
SCL => D22
SDA => D21
Ultrasonic sensor:
Vcc => 5V
GND => GND
Trig => D23
Echo => D4
Push button:
One pin connected to D2
Other pin connected to GND
Installation:
To use this project, you will need to install the following software:

Arduino IDE
MQTT Broker (such as Mosquitto or HiveMQ)
To install the software, follow these steps:
Download and install the Arduino IDE from the official website.
Connect the ESP32 microcontroller to your computer via USB.
Open the Arduino IDE and select the correct board and port.
Download the necessary libraries for the AHT10 sensor, ultrasonic sensor, and MQTT protocol.
You can find these libraries in the Arduino Library Manager or on Github.
Open the project sketch in the Arduino IDE and upload it to the ESP32.
Set up an MQTT broker on your local network or cloud service, and configure the ESP32 to connect to it.
Usage:
Once the project is installed, it will automatically begin collecting data from the AHT10 sensor
every five minutes and transmitting it to the MQTT server.
The data can be viewed and analyzed using an MQTT client or custom software.
Press the push button to switch between display screens for temperature, humidity, volume, and internal temperature.

Troubleshooting:
If you encounter any issues with the project, try the following steps:

Check your wiring connections and ensure they are correct.
Verify that the necessary libraries are installed and up-to-date.
Check that the ESP32 is connected to the correct Wi-Fi network and that the MQTT broker is running.
Debug the project code by using serial output or adding breakpoints in the code.
Conclusion:
The AHT10 temperature and humidity monitoring system is a useful tool for collecting and transmitting environmental data. With the help of an ESP32 microcontroller and an MQTT server, this system can be easily set up and configured for a variety of applications.
*/
#include <AHTxx.h> // Include library for the AHTxx temperature and humidity sensor
#include <stdbool.h>
#include <stdint.h>
#include <WiFi.h> // Include library for WiFi connectivity
#include <ESPmDNS.h> // Include library for mDNS
#include <WiFiUdp.h> // Include library for UDP communication
#include <string.h>
#include <ArduinoOTA.h> // Include library for OTA updates
#include "aurdino_secret.h" // Include file containing sensitive information such as passwords and network credentials
#include <PubSubClient.h> // Include library for MQTT communication
#include "driver/gpio.h"

//-------------screen variable-----------------------
// Include necessary libraries for the OLED display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>

// Define constants for the OLED display, including its width, height, reset pin, and I2C address
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

// Create an instance of the Adafruit_SSD1306 class for the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
   
//---------------------------------------------------
unsigned long previousMillis = 0;  // variable to store the last time the LED was updated
const long interval = 1000;        // interval at which to blink the LED (milliseconds)

//----------internal temp---------------
// Include necessary libraries for reading the internal temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// Define the GPIO pin to which the temperature sensor is connected
#define ONE_WIRE_BUS GPIO_NUM_18

// Create a OneWire instance to communicate with the temperature sensor using the specified GPIO pin
OneWire oneWire(ONE_WIRE_BUS);

// Create a DallasTemperature instance that uses the OneWire instance to communicate with the temperature sensor
DallasTemperature sensors(&oneWire);

//-------------------------------------------------
//----------------button variable----------------

// Define constants and variables for the button
#define D2 GPIO_NUM_2
volatile bool buttonPressed = false;


// bool debounce() {
//   static uint16_t state = 0;
//   state = (state<<1) | digitalRead(D2) | 0xfe00;
//   return (state == 0xff00);
// }


// Define an ISR (Interrupt Service Routine) for the button
void IRAM_ATTR button_ISR() {
  Serial.println("buttun pressed!"); // Print a message to the serial monitor indicating that the button has been pressed
  buttonPressed = true; // Set the buttonPressed flag to true
  Serial.println(buttonPressed); // Print the value of the buttonPressed flag to the serial monitor
}
//----------------ultrasonic variable----------------
// Include necessary libraries for the ultrasonic sensor
#include <Ultrasonic.h>

// Define constants and variables for the ultrasonic sensor
const int trigPin = 23;
const int echoPin = 4;

// Define constants for the dimensions of the water tank
const float sensorHeight = 0.1; // The height of the ultrasonic sensor above the water level
const float majorAxisLength = 1.528; // The length of the major axis of the oval cross-section
const float minorAxisLength = 1.155; // The length of the minor axis of the oval cross-section
const float cylinderLength = 2.921; // The length of the cylinder

// Create an instance of the Ultrasonic class for the ultrasonic sensor
Ultrasonic ultrasonic(trigPin, echoPin);

// Define a function and an integration function for calculating the volume of water in the tank
double function(double x) {
  return majorAxisLength/minorAxisLength*sqrt(pow(minorAxisLength,2)-pow(x-minorAxisLength,2));
}

double integral(double a, double b, int n) {
  double h = (b - a) / n;
  double sum = 0;
  for (int i = 0; i < n; i++) {
    double x = a + h * i;
    sum += function(x) * h;
  }
  return sum;
}

//---------------------------------------------------
// Define constants and variables for the LED and the Wi-Fi network
#define LED_GPIO GPIO_NUM_5
#define LED_ON 0
#define LED_OFF 1
const char* ssid = SSID_home;
const char* password = SSID_PASS;
// Define constants and variables for general use
int scl = 15; // GPIO pin for I2C clock
int sda = 2; // GPIO pin for I2C data
AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR); // Create an instance of the AHTxx class for the temperature and humidity sensor
float ahtValue; // Variable to store the temperature and humidity values read from the sensor
char *list_of_strings[4] = {
        "HA/greenhouse/temperature",
        "HA/greenhouse/humidity",
        "HA/milkContainer/volume",
        "HA/milkContainer/temp"
    }; // Array of strings for publishing MQTT messages
float list_of_floats[4]; // Array of floats for storing sensor data
int size = 2; // The number of elements in the array to publish
char* mqttServer = "192.168.31.130"; // The IP address of the MQTT broker
int mqttPort = 1883; // The port number of the MQTT broker
char* mqttUser = "home"; // The username for connecting to the MQTT broker
char* mqttPassword = "089923743"; // The password for connecting to the MQTT broker

WiFiClient wifiClient; // Create a WiFiClient instance for connecting to the Wi-Fi network
PubSubClient client(wifiClient); // Create a PubSubClient instance for publishing MQTT messages
volatile int buttonPress = 0; // A flag for indicating whether the button has been pressed

// 'cow', 64x64px
const unsigned char splash_data [] PROGMEM = {
// 'cow, 128x64px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x7c, 0x3e, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xf0, 0xe0, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xe0, 0xe0, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0e, 0x00, 0x00, 0x00, 0x3d, 0xf0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0c, 0x00, 0x00, 0x00, 0x1c, 0x70, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x0c, 0x00, 0x00, 0x00, 0x0e, 0x38, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x01, 0xff, 0x06, 0x18, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x01, 0xff, 0x87, 0x1c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xff, 0xc7, 0x1c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x01, 0xc7, 0x0c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0xc7, 0x0c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x07, 0x0c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x07, 0x1e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0x07, 0x1e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x07, 0x1e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x1f, 0xff, 0xf8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x1f, 0xff, 0xfc, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x1c, 0x78, 0x1f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x18, 0x3c, 0x0f, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x18, 0x3f, 0xbf, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x18, 0x3f, 0xfd, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x18, 0x33, 0xf9, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x18, 0x03, 0x99, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x38, 0x03, 0x99, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x38, 0x01, 0x19, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x38, 0x00, 0x19, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x38, 0x00, 0x19, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x38, 0x00, 0x19, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//-----------------------------------------
void setup() {
  // Initialize the OLED display and show the splash screen
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
    // Set the text parameters for the display
  display.setTextWrap(false); // Disable text wrapping
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.cp437(true);

  // Display the WiFi status on the first line of the display
  display.setCursor(0,2);
  display.print("booting...");
  display.display();
  delay(1000);
    // Start the serial communication for debugging
  Serial.begin(115200);
  Serial.println("Booting");

    // Connect to the Wi-Fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
    display.clearDisplay();
    // Set the text parameters for the display
  display.setTextWrap(true); // Disable text wrapping
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.cp437(true);

  // Display the WiFi status on the first line of the display
  display.setCursor(0,15);
  display.println("connected to: ");
  display.println(ssid);
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  
  
  // Configure OTA updates
    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  //---------------------------------setup-------------------------------
  
  // Set up the MQTT client
  client.setServer(mqttServer, 1883);

  // Initialize the AHT10 temperature and humidity sensor
  if (!aht10.begin()) {
      Serial.println("Failed to initialize AHT10 sensor!");
  }

  // Set up the LED pin as an output
  pinMode (LED_GPIO, OUTPUT);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Set up the button pin as an input with pull-up resistor
  pinMode(D2, INPUT_PULLUP);
  
  // Attach an interrupt to the button pin for detecting button presses
  attachInterrupt(digitalPinToInterrupt(D2), button_ISR, FALLING);

  // Initialize the DallasTemperature library for the internal temperature sensor
  sensors.begin();

  int i;
  for (i=128-32;i>-64-32;i--){
    display.clearDisplay();
    display.drawBitmap(i, 0, splash_data, 128, 64, WHITE);
    display.display();
    delay(40);

  }

  // Wait for a few seconds to display the splash screen
  //delay(3000);

  // Clear the display and proceed with the rest of the program
  display.clearDisplay();
  display.display();

}
//---------------------------------------------------------------------

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client",mqttUser,mqttPassword)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * This function publishes a float value to an MQTT topic and prints it to the serial console.
 * 
 * @param i The index of the float value and the MQTT topic string in the global arrays.
 */
void printandsend(int i) {
  // Convert the float value to a string with two decimal places
  char tempString[8];
  dtostrf(list_of_floats[i], 1, 2, tempString);

  // Print the MQTT topic and the float value to the serial console for debugging
  Serial.print(list_of_strings[i]);
  Serial.print("\t");
  Serial.println(tempString);

  // Publish the float value to the MQTT topic using the PubSubClient library
  client.publish(list_of_strings[i], tempString);
}

/**
 * This function displays a title and a float value with a unit suffix on the OLED display.
 * 
 * @param Title The title to display on the first line of the display.
 * @param number The float value to display on the third line of the display.
 * @param suffix The ASCII code of the unit suffix to display after the float value.
 */
void uniqeDisp(String Title, float number, int suffix) {
  // Clear the OLED display
  display.clearDisplay();

  // Set the text parameters for the display
  display.setTextWrap(false); // Disable text wrapping
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.cp437(true);

  // Display the WiFi status on the first line of the display
  display.setCursor(0,2);
  display.print("WiFi:");
  if (WiFi.status() == WL_CONNECTED) {
    display.print(WiFi.localIP());
  } else {
    display.print("Disconnected");
  }

  // Display the title on the second line of the display
  display.setCursor(0,16);
  display.setTextSize(2);
  display.println(Title);

  // Display the float value and the unit suffix on the third line of the display
  display.setTextSize(3);
  display.setCursor(0,35);
  display.print(number);
  display.write(suffix);

  // Update the OLED display
  display.display();
}


void loop() {
  ArduinoOTA.handle();
  
  //----------------------------loop--------------------------------------
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval*10) {   // check if it's time to blink the LED
    previousMillis = currentMillis;   // save the last time the LED was updated
    if (!client.connected()) {
    reconnect();
    }
    client.loop();
    
    digitalWrite (LED_GPIO, LOW);

    ahtValue = aht10.readTemperature(); //read 6-bytes via I2C, takes 80 milliseconds

  
    if (ahtValue != AHTXX_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
    {
      list_of_floats[0] = ahtValue;
    }
    else  {
      digitalWrite (LED_GPIO, HIGH);
      printStatus(); //print temperature command status
      list_of_floats[0] = 0.0;

      if   (aht10.softReset() == true) Serial.println(F("reset success")); //as the last chance to make it alive
      else                             Serial.println(F("reset failed"));
    }
    printandsend(0);

    ahtValue = aht10.readHumidity(); //read another 6-bytes via I2C, takes 80 milliseconds

    
    if (ahtValue != AHTXX_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
    { 
      list_of_floats[1] = round(ahtValue*10)/10;
    }
    else
    {
      digitalWrite (LED_GPIO, HIGH);
      printStatus(); //print humidity command status
      list_of_floats[1] = 0;
    }
    printandsend(1);
    digitalWrite (LED_GPIO, LOW);
    //---------------ultrasonic part------------------------
    // Measure the distance to the water surface in cm
    float distance = ultrasonic.read();

    // Convert the distance to meters
    distance /= 100.0;

    // Calculate the height of the water level in the container
    float waterHeight = distance - sensorHeight;

    // Calculate the volume of water in the container
    float volume = waterHeight * (3.14159 * majorAxisLength * minorAxisLength) / 4 * cylinderLength;
    double result = integral(0,minorAxisLength-distance,9) * cylinderLength * 2 * 1000 ;
    list_of_floats[2]=result;
    printandsend(2);
    //---------------internal temp part------------------------
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    list_of_floats[3]=temperature;

    printandsend(3);
    buttonPressed = true;
  }
  
    // check if button has been pressed
  if (buttonPressed == true) {
    Serial.println(buttonPress);
    buttonPress++;
    if (buttonPress==4)
      buttonPress = 0;
    buttonPressed = false;
  }
  
  // other loop code here
  switch (buttonPress){
    case 0:
      uniqeDisp("Temperature", list_of_floats[0], 248);
      break;
    case 1:
      uniqeDisp("Humidity", list_of_floats[1], 37);
      break;
    case 2:
      uniqeDisp("Liters", list_of_floats[2], 76);
      break;
    case 3:
      uniqeDisp("Temperature", list_of_floats[3], 248);
      break;
  }
   
}

void printStatus()
{
  switch (aht10.getStatus())
  {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));    
      break;
  }
}