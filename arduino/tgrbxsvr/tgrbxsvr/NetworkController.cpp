#include "NetworkController.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SD.h>

const String frontendRootPath = "/frontend/";

WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int CLEAN_CLIENTS_INTERVAL = 10;  //seconds
TaskHandle_t clearClientsTaskHandle;

void clearClientsTask(void *pvParameters) {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(CLEAN_CLIENTS_INTERVAL * 1000));
    ws.cleanupClients();
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
  }
}

//-- Rest API handlers --//

void handleApiActivateSchedule(AsyncWebServerRequest *request) {
  Serial.println("Handle activate schedule request");

  if (!request->hasParam("id") || !request->hasParam("active")) {
    request->send(400);
  }

  AsyncWebParameter *pId = request->getParam("id");
  AsyncWebParameter *pActive = request->getParam("active");

  try {
    const int id = std::stoi(pId->value().c_str());
    const bool active = (std::stoi(pActive->value().c_str()) != 0);

    dataAccess.setActiveSchedule(id, true);

    if (selectedSchedule != nullptr) {
      if (selectedSchedule->ID != id) {
        dataAccess.setSelectSchedule(selectedSchedule->ID, false);
        selectedSchedule = dataAccess.getSelectedSchedule();
      } else {
        selectedSchedule->Active = active;
      }
    } else {
      selectedSchedule = dataAccess.getSelectedSchedule();
    }
  } catch (...) {
    request->send(400);
  }
}

//---//

bool NetworkController::initNetworkConnection(Config config) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(config.ssid, config.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Start MDNS...");
  const char *domainName = config.localDomainName != nullptr ? config.localDomainName : "futterspender";
  if (MDNS.begin(domainName)) {
    Serial.println("MDNS started!");
  } else {
    Serial.println("Couldn't start MDNS");
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

bool NetworkController::initWebserver() {
  server.on("/api/container", [](AsyncWebServerRequest *request) {
    //handleApiContainer(request);
  });
  server.on("/api/settings", [](AsyncWebServerRequest *request) {
    //handleApiSettings(request);
  });
  server.on("/api/schedule", [](AsyncWebServerRequest *request) {
    //handleApiSettings(request);
  });
  server.on("/api", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "application/json", "{\"message\":\"Not found\"}");
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("Web Server started");
  xTaskCreate(clearClientsTask, "ClearClientsTask", 4096, NULL, 1, &clearClientsTaskHandle);
  return true;
}

bool NetworkController::hasWebClients() {
  return ws.count() > 0;
}

void NetworkController::broadcast(const char *serializedMessage) {
  ws.textAll(serializedMessage);
}