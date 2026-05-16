#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "TP-LINK_EB30";
const char* password = "PondRat1";

WebServer server(80);

const int sensorPin = 1;

int dryValue = 3775;
int wetValue = 1335;

void handleRoot() {
  int value = analogRead(sensorPin);

  int moisturePercent = map(value, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  String page = "<html>";
  page += "<head>";
  page += "<meta http-equiv='refresh' content='5'>";
  page += "<style>";
  page += "body { font-family: Arial; text-align: center; padding-top: 40px; }";
  page += "h1 { font-size: 36px; }";
  page += "p { font-size: 28px; }";
  page += "</style>";
  page += "</head>";
  page += "<body>";
  page += "<h1>Garden Monitor</h1>";
  page += "<p>Moisture: ";
  page += moisturePercent;
  page += "%</p>";
  page += "<p>Raw: ";
  page += value;
  page += "</p>";
  page += "</body>";
  page += "</html>";

  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}