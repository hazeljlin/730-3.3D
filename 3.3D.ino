#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>  // For HTTPS request to IFTTT

// WiFi credentials
char ssid[] = "TP-Link_38E2";
char pass[] = "40377506";

// MQTT broker info
const char* mqtt_server = "76f26905b7ad439195b68a0b1d15f868.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

// IFTTT webhook details
const char* ifttt_host = "maker.ifttt.com";
const int httpsPort = 443;
const char* ifttt_event = "wave_detected";
const char* ifttt_key = "Lw_hsjG8BXGtq715socge";
String ifttt_url = "/trigger/" + String(ifttt_event) + "/with/key/" + String(ifttt_key);

// Topics
const char* wave_topic = "SIT730/wave";
const char* pat_topic = "SIT730/pat";

// Pins
const int trigPin = 2;
const int echoPin = 3;
const int ledPin = 5;

// Clients
WiFiSSLClient wifiClient;
PubSubClient client(wifiClient);
HttpClient http(wifiClient, ifttt_host, httpsPort);

// State
unsigned long lastMsg = 0;
const long interval = 2000;
int waveCount = 0;

void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void sendIFTTTAlert() {
  Serial.println("Sending IFTTT request...");
  http.beginRequest();
  http.get(ifttt_url);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  Serial.print("IFTTT Status Code: ");
  Serial.println(statusCode);
  Serial.println("Response: " + response);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "wave") {
    Serial.println("Blinking slowly 3 times");
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
      delay(500);
    }
  } else if (message == "pat") {
    Serial.println("Blinking quickly 5 times");
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, HIGH);
      delay(150);
      digitalWrite(ledPin, LOW);
      delay(150);
    }
  } else {
    Serial.println("Unknown message type");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ArduinoClient", "hivemq.webclient.1744030978372", "fAI<S&9.jo36Q,Kl2wvX")) {
      Serial.println("connected");
      client.subscribe(wave_topic);
      client.subscribe(pat_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    long distance = getDistance();
    Serial.print("Distance: ");
    Serial.println(distance);

    if (distance < 15) {
      Serial.println("Wave detected!");
      client.publish(wave_topic, "wave");

      waveCount++;
      Serial.print("Wave Count: ");
      Serial.println(waveCount);

      if (waveCount >= 10) {
        sendIFTTTAlert();
        waveCount = 0; // Reset counter
      }

      delay(1000);
    }
  }
}

