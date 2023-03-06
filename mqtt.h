#include <WiFi.h>
#include <string.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

const char* mqttServer = "192.168.31.130";
const int mqttPort = 1883;
const char* mqttUser = "home";
const char* mqttPassword = "089923743";
const char* topic = "HA/greenhouse";
WiFiClient espClient;
PubSubClient client(espClient);


void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message received: ");
  Serial.println(message);
}


int mqttConnect(){
  // Connect to MQTT broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMqttMessage);
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT broker");
      return 1;
    } else {
      Serial.println("Connection failed, rc = " + String(client.state()));
      delay(2000);
      return 0;
    }
  }
}

int mqttPublish(char **strings, float *floats, int size){
  // Publish a message

  StaticJsonDocument<200> doc;
  int i;
  for (i = 0; i < size; i++) {
    doc[strings[i]]=round(floats[i]*100)/100;
    }
  //doc["temperature"] = 4.2;
  //doc["volume"] = result;
  String json;
  serializeJson(doc, json);
  Serial.println(json);

  if (!client.publish(topic, json.c_str())) {
    Serial.println("Publish failed");
    return 0;
  } else {
    Serial.println("Message published");
    return 1;
  }
}