// Import required libraries
#include <WiFi.h>
//#include <AsyncTCP.h>
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ESPmDNS.h>
#include <Servo.h>
#include <HX711.h>

struct Config {
  char ssid[64];
  char password[64];
};

struct Notification{
  bool Active;
  bool Email;
  bool Phone;
};

struct NotificationList{
  Notification ContainerEmpty;
  Notification DidNotEatInADay;
};

struct Settings {
  String PetName;
  double PlateTAR;
  double PlateFilling;
  NotificationList Notifications;
  String Email;
  String Phone;
  int Language;
  int Theme;
};

const char* configPath = "/config.json";
Config config;

const String frontendRootPath = "/frontend/";
const String dataRootPath = "/data/";

double test_weight = 315;
Settings settings;

WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const int servoPin = 22;
Servo servo;

// HX711 circuit wiring
const int LOADCELL_TIMES = 10;

const int LOADCELL_A_SCK_PIN = 27;
const int LOADCELL_A_DOUT_PIN = 26;
HX711 scale_A;

const int LOADCELL_B_SCK_PIN = 25;
const int LOADCELL_B_DOUT_PIN = 33;
HX711 scale_B;

double getScaleValue(HX711 scale){
  long val = scale.get_units(LOADCELL_TIMES) * 10;
  double scaleValue =  (double) val / 10;
  //Serial.println(scaleValue);
  return scaleValue;
}

void handleApiTime(){
  timeClient.update();

  String formattedDate = timeClient.getFormattedTime();
  String response = "Api Time: " + formattedDate;
  server.send(200, "text/html", response);
}

void handleApiContainer(){
  Serial.println("Handle container request");
  server.send(200);
  /*
  if(request->hasParam("open")){
    AsyncWebParameter* p = request->getParam("open");
    if(p->value() != "true"){
      servo.write(90);
    }else{
      servo.write(0);
    }
    server.send(200);
  }else{
    servo.write(0);
    server.send(400);
  }*/
}

void handleApiStatus(){
 // Serial.println("Handle status request");

  String response;
  DynamicJsonDocument doc(1024);

  double containerLoad = getScaleValue(scale_A);
  double plateLoad = getScaleValue(scale_B);

  doc["ContainerLoad"] = containerLoad;
  doc["PlateLoad"] = plateLoad;
  doc["SDCardConnection"] = true;
  doc["MotorOperation"] = true;
  doc["WiFiConnection"] = true;

  serializeJson(doc, response);

  server.send(200, "application/json", response);
}

void handleApiSettings(){
  Serial.println("Handle scale request");
  server.send(200);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  
  //HX711 Setup
  Serial.println("Setting up scale A...");
  scale_A.begin(LOADCELL_A_DOUT_PIN, LOADCELL_A_SCK_PIN);

  while(!scale_A.is_ready()){
    Serial.println(".");
    delay(2000);
  }

  scale_A.tare(LOADCELL_TIMES); 
  Serial.println("\nPut 152 gram on scale A, press a key to continue");
  while(!Serial.available());
  while(Serial.available()) Serial.read();
  scale_A.calibrate_scale(152, 5);
  Serial.println("Scale A ready.");
  
  Serial.println("Setting up scale B...");
  scale_B.begin(LOADCELL_B_DOUT_PIN, LOADCELL_B_SCK_PIN);

  while(!scale_B.is_ready()){
    Serial.println(".");
    delay(2000);
  }

  scale_B.tare(LOADCELL_TIMES); 
  Serial.println("\nPut 152 gram on scale B, press a key to continue");
  while(!Serial.available());
  while(Serial.available()) Serial.read();
  scale_B.calibrate_scale(152, 5); 
  Serial.println("Scale B ready.");
  
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

  Serial.println("Start mDNS...");
  if (MDNS.begin("futterspender")) {
    Serial.println("MDNS started!");
  }else{
    Serial.println("Couldn't start mdns");
  }
  
  //Serial.println("Start servo");
  //servo.attach(servoPin);

  server.on("/api/time", [](){
    handleApiTime();
  });
  server.on("/api/container", [](){
    handleApiContainer();
  });
  server.on("/api/status", [](){
    handleApiStatus();
  });
  server.on("/api/settings", [](){
    handleApiSettings();
  });
  server.on("/api", [](){
    server.send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str());//.setDefaultFile("index.html");

  server.onNotFound([](){
    if (server.method() == HTTP_OPTIONS) {
      server.send(200);
    } else {
      server.send(404, "application/json", "{\"message\":\"Not found\"}");
    }
  });
  
  /*DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");*/
  server.enableCORS(true);
  server.begin();
  Serial.println("Web Server started");

  timeClient.begin();
  timeClient.update();
  /*
  Serial.println("Init DB...");

  sqlite3 *db1;
  char *zErrMsg = 0;
  int rc;

  sqlite3_initialize();

  if (openDb("/sd/data/test.db", &db1)){
    return;
  }

  rc = db_exec(db1, "Select * from data where ID = 1");
  if (rc != SQLITE_OK) {
       //sqlite3_close(db1);
       //return;
       //TODO: error handling
  }

  sqlite3_close(db1);*/
}

void loop() {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

int openDb(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
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