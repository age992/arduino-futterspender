// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "antons_hotspot";
const char* password = "KeckPeter123#";
const String frontendRootPath = "/frontend/";

// Create AsyncWebServer object on port 80
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
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  server.on("/api/time", [](AsyncWebServerRequest *request){
    handleApiTime(request);
  });
  server.on("/api", [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  // Start server
  server.begin();
  Serial.println("Web Server started");

  timeClient.begin();
  timeClient.update();

  if (SD.begin()) {
    Serial.println("SD card initialization successfull");
  }else{
    Serial.println("SD card initialization failed!");
  }
}

void loop() {

}