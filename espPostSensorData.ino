// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// WIFI login data
const char WIFI_SSID[] = "WIFI-Z77IDEY";         
const char WIFI_PASSWORD[] = ""; 

// url of java backend
String HOST_NAME   = "http://10.1.10.13:8080"; 

// humidity and temperature sensor data
#define DHTPIN 2     
#define DHTTYPE DHT22     
DHT_Unified dht(DHTPIN, DHTTYPE);


// time related constants
const char* ntpServer = "pool.ntp.org";
const long utcOffsetInSeconds = 3600; // UTC +1 (für Mitteleuropa)

// sensor reading interval
//uint32_t delayMS = 900000; // 15 minutes
uint32_t delayMS = 60000; // 1 minute


void setup() {
  Serial.begin(9600);
  

  //setup wifi data
  Serial.println("wifi setup");
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    Serial.println("WiFi not connected!");
    delay(10000);
  }

  //setup dht22 sensor
  Serial.println("dht22 setup");
  sensor_t sensor;
  dht.begin();
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

  Serial.println("Setup finished, moving to loop");
  
  //delayMS = sensor.min_delay / 1000;

  // NTP-Server synchronisieren
  configTime(utcOffsetInSeconds, 0, ntpServer);
}

void loop() {

  HTTPClient http;
  //get current timestamp from time-server
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char buffer[20]; 
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  Serial.println("current timestamp : " + String(buffer));

  //read temperature data from dht22 sensor
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));

    // send data to java backend via post request
    http.begin(HOST_NAME + "/temperature");
    http.addHeader("Content-Type", "application/json");
    String payload = "{";
    payload += "\"value\":\"" + String(event.temperature) + "\",";
    payload += "\"timestamp\":\"" + String(buffer) + "\"";
    payload += "}";
    int httpCode = http.POST(payload);

    http.end();
    Serial.print("HTTP Return Code of POST-Request: ");
    Serial.println(httpCode);
  }


  // read humidity data from dht22 sensor
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(event.relative_humidity);
    
    http.begin(HOST_NAME + "/humidity");
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"value\":\"" + String(event.relative_humidity) + "\",";
    payload += "\"timestamp\":\"" + String(buffer) + "\"";
    payload += "}";

    int httpCode = http.POST(payload);
    http.end();
    Serial.print("HTTP Return Code of POST request: ");
    Serial.println(httpCode);
  }

  Serial.print("Waiting for ");
  Serial.print(delayMS/1000);
  Serial.println("seconds until next interval");
  delay(delayMS);
}