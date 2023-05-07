#include "NetworkController.h"
#include "freertos/FreeRTOS.h"
#include <AsyncTCP.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SD.h>

const String frontendRootPath = "/frontend/";

WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp);

AsyncWebServer server(80);

bool NetworkController::initNetworkConnection(Config config) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(config.ssid, config.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Start mDNS...");
  const char* domainName = config.localDomainName != nullptr ? config.localDomainName : "futterspender";
  if (MDNS.begin(domainName)) {
    Serial.println("MDNS started!");
  } else {
    Serial.println("Couldn't start mdns");
  }
  return true;
}

bool NetworkController::initNTP() {
  ntpClient.begin();
  return true;
}

long NetworkController::getCurrentTime() {
  ntpClient.update();
  return ntpClient.getEpochTime();
}

bool NetworkController::initWebserver(){
  server.on("/api/time", [](AsyncWebServerRequest *request){
    //handleApiTime(request);
  });
  server.on("/api/container", [](AsyncWebServerRequest *request){
    //handleApiContainer(request);
  });
  server.on("/api/status", [](AsyncWebServerRequest *request){
    //handleApiStatus(request);
  });
  server.on("/api/settings", [](AsyncWebServerRequest *request){
    //handleApiSettings(request);
  });
  server.on("/api", [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request){
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "application/json", "{\"message\":\"Not found\"}");
    }
  });
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server.begin();
  Serial.println("Web Server started");
  return true;
}