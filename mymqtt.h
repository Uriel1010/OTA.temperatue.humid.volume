#include <WiFi.h>
#include <string.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

//WiFiClient espClient;
//PubSubClient client(espClient);

typedef struct mqtt_t {
  const char *server;
  int port;
  const char *user;
  const char *password;
  const char *topic;
  //WiFiClient *espClient;
  PubSubClient client;
} mqtt_t;

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message received: ");
  Serial.println(message);
}

int mqttConnect(mqtt_t *mqtt){
  int retries = 0;
  int max_retries = 5;

  // Connect to MQTT broker
  mqtt->client.setServer(mqtt->server, mqtt->port);
  Serial.println("1");  
  mqtt->client.setCallback(onMqttMessage);
  Serial.println("2");  
   while (!mqtt->client.connected()) {
     Serial.println("3");
     Serial.println(mqtt->client.state());  
     if (mqtt->client.connect("ESP32Client", mqtt->user, mqtt->password)) {
       Serial.println("Connected to MQTT broker");
       return 1;
     } else {
       retries++;
       Serial.println("Connection failed, rc = " + String(mqtt->client.state()));
       if (retries == max_retries) {
         return 0;
       }
       delay(2000);
     }
   }
}

int mqttPublish(mqtt_t *mqtt, char **strings, float *floats, int size){
  // Publish a message

  if (mqtt->client.connected()) {
    StaticJsonDocument<200> doc;
    int i;
    for (i = 0; i < size; i++) {
      doc[strings[i]]=round(floats[i]*100)/100;
    }
    String json;
    serializeJson(doc, json);
    Serial.println(json);

    if (!mqtt->client.publish(mqtt->topic, json.c_str())) {
      Serial.println("Publish failed");
      return 0;
    } else {
      Serial.println("Message published");
      return 1;
    }
  } else {
    Serial.println("MQTT client not connected");
    return 0;
  }
}
