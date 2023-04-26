// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

struct Config {
  char ssid[64];
  char password[64];
};

const char* ssid = "antons_hotspot";
const char* password = "KeckPeter123#";
const char* configPath = "/config.json";
Config config;        
const String frontendRootPath = "/frontend/";

AsyncWebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void handleApiTime(AsyncWebServerRequest *request){
  timeClient.update();

  String formattedDate = timeClient.getFormattedTime();
  String response = "Api Time: " + formattedDate;
  request->send(200, "text/html", response);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  //init SD card
  while (!SD.begin()) {
    Serial.println("Couldn't initialize SD Card. Retry in 5 seconds...");
    delay(5000);
  }
    
  Serial.println("SD card initialized.");
  Serial.println("Loading config.");
  
  //Load config
  while(!loadConfiguration(configPath, config)){
    Serial.println("Couldn't read config file. Retry in 5 seconds...");
    delay(5000);
  }

  Serial.println("Config loaded.");
  Serial.print("SSID: ");
  Serial.println(config.ssid);
  Serial.print("Password: ");
  Serial.println(config.password);

  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(config.ssid, config.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  server.on("/api/time", [](AsyncWebServerRequest *request){
    handleApiTime(request);
  });
  server.on("/api", [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.begin();
  Serial.println("Web Server started");

  timeClient.begin();
  timeClient.update();
}

void loop() {

}

bool loadConfiguration(const char *filename, Config &config) {
  if(!SD.exists(filename)){
     Serial.println(F("Config file not found"));
     return false;
  }

  File file = SD.open(filename);

  StaticJsonDocument<512> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println(F("Failed to deserialize config file"));
    return false;
  };

  strlcpy(config.ssid, doc["ssid"], sizeof(config.ssid));
  strlcpy(config.password, doc["password"], sizeof(config.password));

  file.close();

  return config.ssid != "" && config.password != "";
}