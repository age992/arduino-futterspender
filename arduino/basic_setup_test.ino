#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define ESP8266_RX_PIN 2
#define ESP8266_TX_PIN 3

// Replace with your network credentials
const char *ssid = "your_SSID";
const char *password = "your_PASSWORD";

SoftwareSerial espSerial(ESP8266_RX_PIN, ESP8266_TX_PIN);
ESP8266WebServer server(80);

void setup()
{
    Serial.begin(9600);
    espSerial.begin(9600);

    Serial.println("Sending AT commands to ESP8266");
    espSerial.println("AT");
    delay(1000);
    espSerial.println("AT+CWMODE=1");
    delay(1000);
    espSerial.println("AT+CWJAP=\"SSID\",\"PASSWORD\"");
    delay(5000);

    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");

    server.on("/", handleRoot);
    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();
}

void handleRoot()
{
    String html = "<html><body><h1>Hello, World!</h1></body></html>";
    server.send(200, "text/html", html);
}
