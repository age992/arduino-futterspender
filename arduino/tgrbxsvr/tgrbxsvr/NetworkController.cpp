#include <string>
#include <vector>
#include "NetworkController.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "JsonHelper.h"

const String frontendRootPath = "/frontend/";

WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp);
long startUpTimestamp = 0;
long startUpDaytimeMS = 0;
int today = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int CLEAN_CLIENTS_INTERVAL = 10;  //seconds
TaskHandle_t clearClientsTaskHandle;

void clearClientsTask(void *pvParameters) {
  while (true) {
    //Serial.println("Cleanup clients");
    vTaskDelay(pdMS_TO_TICKS(CLEAN_CLIENTS_INTERVAL * 1000));
    ws.cleanupClients();
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      Serial.println("Data ws received");
      //Serial.printf(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

//-- Rest API handlers --//

void handleApiScheduleActivate(AsyncWebServerRequest *request) {
  //Serial.println("Handle activate schedule request");

  if (!request->hasParam("id") || !request->hasParam("active")) {
    request->send(400);
  }

  AsyncWebParameter *pId = request->getParam("id");
  AsyncWebParameter *pActive = request->getParam("active");

  try {
    const int id = std::stoi(pId->value().c_str());
    const bool active = pActive->value().equals("true");

    if (selectedSchedule != nullptr && selectedSchedule->ID != id) {
      dataAccess.setSelectSchedule(selectedSchedule->ID, false);
      dataAccess.setActiveSchedule(selectedSchedule->ID, false);
    }

    dataAccess.setSelectSchedule(id, true);
    dataAccess.setActiveSchedule(id, active);

    Schedule *newSchedule = dataAccess.getSelectedSchedule();
    setSchedule(newSchedule);

    Serial.println("Selected Schedule: ");
    if (selectedSchedule == nullptr) {
      Serial.println("nullptr");
    } else {
      Serial.println(selectedSchedule->Name);
    }

    request->send(200);
  } catch (...) {
    request->send(400);
  }
}

void handleApiScheduleGet(AsyncWebServerRequest *request) {
  if (request->hasParam("id")) {
    try {
      AsyncWebParameter *pId = request->getParam("id");
      const int id = std::stoi(pId->value().c_str());
      Schedule *schedule = dataAccess.getScheduleByID(id);

      if (schedule == nullptr) {
        //return empty response
      } else {
        //return serialized response
      }
    } catch (...) {
      request->send(400);
    }
  } else {
    std::vector<Schedule> schedules;
    dataAccess.getAllSchedules(schedules);
    String response = serializeSchedules(schedules);
    request->send(200, "application/json", response);
  }
}

void handleApiScheduleDelete(AsyncWebServerRequest *request) {
  if (!request->hasParam("id")) {
    request->send(400);
  }
  try {
    AsyncWebParameter *pId = request->getParam("id");
    const int id = std::stoi(pId->value().c_str());
    if (dataAccess.deleteSchedule(id)) {
      if (selectedSchedule != nullptr && selectedSchedule->ID == id) {
        selectedSchedule = nullptr;
      }
      Serial.print("Deleted schedule (ID=");
      Serial.print(std::to_string(id).c_str());
      Serial.println(")");
      request->send(200);
    } else {
      request->send(404);
    }
  } catch (...) {
    request->send(400);
  }
}

void handleApiSchedule(AsyncWebServerRequest *request, uint8_t *data) {
  WebRequestMethodComposite method = request->method();
  Serial.print("Handle Api Schedule ");
  switch (method) {
    case HTTP_POST:
      {
        Serial.println("POST");
        Schedule *schedule = deserializeSchedule((char *)data);
        const int id = dataAccess.insertSchedule(schedule);
        if (id > 0) {
          request->send(200, "application/json", std::to_string(id).c_str());
        } else {
          request->send(500);
        }
        return;
      }
    case HTTP_PUT:
      {
        Serial.println("PUT");
        Schedule *schedule = deserializeSchedule((char *)data);
        if (dataAccess.updateSchedule(schedule)) {
          request->send(200);
          if (schedule->ID == selectedSchedule->ID) {
            setSchedule(schedule);
          }
        } else {
          request->send(500);
        }
        return;
      }
  }
  request->send(400);
}

void handleApiSettingsHistory(AsyncWebServerRequest *request){
   /*if (!request->hasParam("from")
   || !request->hasParam("to")) {
    request->send(400);
    return;
  }

  AsyncWebParameter *pFrom = request->getParam("from");
  const long from = std::stol(pFrom->value().c_str());

  AsyncWebParameter *pTo = request->getParam("to");
  const long to = std::stol(pTo->value().c_str());

  EventType type = -1;

   if (request->hasParam("type"){
     AsyncWebParameter *pType = request->getParam("type");
     type = std::stoi(pType->value().c_str());
   }*/

  // std::vector<Event> schedules;
   //dataAccess.getAllSchedules(schedules);
   //String response = serializeSchedules(schedules);
  // request->send(200, "application/json", response);
}

void handleApiSettingsGet(AsyncWebServerRequest *request) {
  String response = serializeUserSettings(userSettings);
  request->send(200, "application/json", response);
}

void handleApiSettingsPut(AsyncWebServerRequest *request, uint8_t *data) {
  UserSettings *updatedUserSettings = deserializeUserSettings((char *)data);
  if (dataAccess.updateUserSettings(updatedUserSettings)) {
    delete userSettings;
    userSettings = updatedUserSettings;
    request->send(200);
  } else {
    request->send(500);
  }
}

void handleApiContainer(AsyncWebServerRequest *request) {
  if (!request->hasParam("open")) {
    request->send(400);
    return;
  }

  if (currentStatus->AutomaticFeeding) {
    request->send(409);
    return;
  }

  AsyncWebParameter *p = request->getParam("open");
  bool open = p->value().equals("true");

  if (open) {
    openContainer();
    currentStatus->ManualFeeding = true;
  } else {
    closeContainer();
    currentStatus->ManualFeeding = false;
  }

  request->send(200);
}

void handleApiScaleTare() {
  Serial.print("Handle Api scale tare ");
  /*String param = server.arg("scale");

  if (param == "A") {
    Serial.println("of scale A");
    //machineController.
    scale_A.tare(LOADCELL_TIMES);
    systemSettings.ContainerOffset = scale_A.get_offset();
    saveSystemSettings();
    server.send(200);
  } else if (param == "B") {
    Serial.println("of scale B");
    scale_B.tare(LOADCELL_TIMES);
    systemSettings.PlateOffset = scale_B.get_offset();
    saveSystemSettings();
    server.send(200);
  } else {
    Serial.println("failed");
    server.send(400);
  }*/
}

void handleApiScaleCalibration() {
  /*Serial.print("Hanlde Api scale calibration ");
  String param = server.arg("scale");

  if (param == "A") {
    Serial.println("of scale A");
    scale_A.calibrate_scale(systemSettings.CalibrationWeight, 5);
    systemSettings.ContainerScale = scale_A.get_scale();
    saveSystemSettings();
    server.send(200);
  } else if (param == "B") {
    Serial.println("of scale B");
    scale_B.calibrate_scale(systemSettings.CalibrationWeight, 5);
    systemSettings.PlateScale = scale_B.get_scale();
    saveSystemSettings();
    server.send(200);
  } else {
    Serial.println("failed");
    server.send(400);
  }*/
}

//---//

bool NetworkController::initNetworkConnection(Config *config) {
  Serial.print("Connecting to WiFi...");
  Serial.print(config->ssid);
  Serial.print(" ");
  Serial.println(config->password);
  WiFi.begin(config->ssid, config->password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Start MDNS...");
  const char *domainName = config->localDomainName != nullptr ? config->localDomainName : "futterspender";
  if (MDNS.begin(domainName)) {
    Serial.print("MDNS started! Domain Name: ");
    Serial.println(domainName);
  } else {
    Serial.println("Couldn't start MDNS");
  }
  return true;
}

bool NetworkController::initNTP() {
  ntpClient.begin();
  while (!ntpClient.update()) {
    Serial.println("Couldn't fetch time via NTP...");
    delay(4000);
  }
  startUpTimestamp = ntpClient.getEpochTime();
  today = getDay(startUpTimestamp);
  startUpDaytimeMS = (startUpTimestamp - today * 86400L) * 1000;
  return true;
}

long NetworkController::getCurrentDaytime() {
  //@TODO: handle millis overflow and ntp updates!
  /*if(getDay(millis) > DAYS_UNTIL_NTP_UPDATE){
    ntpClient.update();
    startUpMillis = ntpClient.getEpochTime();
  }*/
  return startUpDaytimeMS + millis();
}

int NetworkController::getToday() {
  return today;
};

bool NetworkController::initWebserver() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/api/container", [](AsyncWebServerRequest *request) {
    handleApiContainer(request);
  });
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleApiSettingsGet(request);
  });
  server.on(
    "/api/settings", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleApiSettingsPut(request, data);
    });
  server.on("/api/schedule/activate", [](AsyncWebServerRequest *request) {
    handleApiScheduleActivate(request);
  });
  server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleApiScheduleGet(request);
  });
  server.on("/api/schedule", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    handleApiScheduleDelete(request);
  });
  server.on(
    "/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleApiSchedule(request, data);
    });
  server.on(
    "/api/schedule", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleApiSchedule(request, data);
    });
  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleApiScheduleDelete(request);
  });

  server.on("/api", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text", "Sorry, this page does not exist!");
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  /*
  AsyncCallbackJsonWebHandler *scheduleJsonHandler = new AsyncCallbackJsonWebHandler("/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &jsonObj = json.as<JsonObject>();
    Serial.println("Received Schedule: ");
    jsonObj.prettyPrintTo(Serial);
    // ...
  });
  server.addHandler(scheduleJsonHandler);*/

  server.begin();
  Serial.println("Web Server started");
  //xTaskCreatePinnedToCore(clearClientsTask, "ClearClientsTask", 4096, NULL, 1, &clearClientsTaskHandle, 1); //@TODO: Why causes this AsyncTCP timeout???
  return true;
}

bool NetworkController::hasWebClients() {
  return ws.count() > 0;
}

void NetworkController::broadcast(const char *serializedMessage) {
  if (ws.availableForWriteAll()) {
    ws.textAll(serializedMessage);
  }
}