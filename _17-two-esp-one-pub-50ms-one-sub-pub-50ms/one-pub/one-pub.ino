#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "/usr/local/src/ap_setting.h"

#define DEBUG_PRINT 1

char* topic = "pubtest";

String clientName;

long lastReconnectAttempt = 0;
long lastMsg = 0;
int test_para = 50;
unsigned long startMills;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
IPAddress mqtt_server = MQTT_SERVER;

WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient);


void sendmqttMsg(char* topictosend, String payload)
{

  if (client.connected()) {
    if (DEBUG_PRINT) {
      Serial.print("Sending payload: ");
      Serial.print(payload);
    }

    unsigned int msg_length = payload.length();

    if (DEBUG_PRINT) {
      Serial.print(" length: ");
      Serial.println(msg_length);
    }

    byte* p = (byte*)malloc(msg_length);
    memcpy(p, (char*) payload.c_str(), msg_length);

    if ( client.publish(topictosend, p, msg_length)) {
      if (DEBUG_PRINT) {
        Serial.println("Publish ok");
      }
      free(p);
      //return 1;
    } else {
      if (DEBUG_PRINT) {
        Serial.println("Publish failed");
      }
      free(p);
      //return 0;
    }
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

boolean reconnect()
{
  if (!client.connected()) {

    if (client.connect((char*) clientName.c_str())) {
      if (DEBUG_PRINT) {
        Serial.println("===> mqtt connected");
      }
    } else {
      if (DEBUG_PRINT) {
        Serial.print("---> mqtt failed, rc=");
        Serial.println(client.state());
      }
    }
  }
  return client.connected();
}

void wifi_connect()
{
  if (WiFi.status() != WL_CONNECTED) {
    // WIFI
    if (DEBUG_PRINT) {
      Serial.println();
      Serial.print("===> WIFI ---> Connecting to ");
      Serial.println(ssid);
    }
    delay(10);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int Attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (DEBUG_PRINT) {
        Serial.print(". ");
        Serial.print(Attempt);
      }
      delay(100);
      Attempt++;
      if (Attempt == 150)
      {
        if (DEBUG_PRINT) {
          Serial.println();
          Serial.println("-----> Could not connect to WIFI");
        }
        ESP.restart();
        delay(200);
      }

    }

    if (DEBUG_PRINT) {
      Serial.println();
      Serial.print("===> WiFi connected");
      Serial.print(" ------> IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
}

void setup()
{
  startMills = millis();

  if (DEBUG_PRINT) {
    Serial.begin(74880);
  }

  wifi_connect();

  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

}

void loop()
{
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      long now = millis();
      if (now - lastReconnectAttempt > 200) {
        lastReconnectAttempt = now;
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      long now = millis();
      if (now - lastMsg > test_para) {
        lastMsg = now;
        String payload = "{\"startMills\":";
        payload += (millis() - startMills);
        payload += "}";
        sendmqttMsg(topic, payload);
      }
      client.loop();
    }
  } else {
    wifi_connect();
  }
/*
  if ( millis() > 30000 ) {
    ESP.deepSleep(0);
    delay(250);
  }
  //delay(10);
*/
 
}

