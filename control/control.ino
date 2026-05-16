// Libraries required (install via Arduino Library Manager):
//   - GxEPD2 by Jean-Marc Zingg
//   - Adafruit GFX Library

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// --- WiFi ---
const char* ssid     = "TP-LINK_EB30";
const char* password = "PondRat1";

WebServer server(80);

// --- Moisture sensor ---
const int sensorPin = 1;  // GPIO1 (analog)
int dryValue = 3775;
int wetValue  = 1335;

// --- E-paper pins (ESP32-C3 Mini) ---
// Wiring:
//   Display VCC  -> 3.3V
//   Display GND  -> GND
//   Display DIN  -> GPIO6  (SPI MOSI)
//   Display CLK  -> GPIO4  (SPI CLK)
//   Display CS   -> GPIO7
//   Display DC   -> GPIO3
//   Display RST  -> GPIO2
//   Display BUSY -> GPIO10
#define EPD_CS   7
#define EPD_DC   3
#define EPD_RST  2
#define EPD_BUSY 10

// Waveshare 2.9" 296x128 B/W (GDEM029T94 chip — the common V2 variant).
// If your display uses a different chip, change GxEPD2_290_T94 to the
// matching class from the GxEPD2 examples (e.g. GxEPD2_290 for V1).
GxEPD2_BW<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT>
  display(GxEPD2_290_T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// --- Display refresh interval ---
#define DISPLAY_INTERVAL_MS (5UL * 60UL * 1000UL)  // 5 minutes
unsigned long lastDisplayUpdate = 0;

// -----------------------------------------------------------------------

int readMoisture(int* rawOut) {
  int raw = analogRead(sensorPin);
  if (rawOut) *rawOut = raw;
  int pct = map(raw, dryValue, wetValue, 0, 100);
  return constrain(pct, 0, 100);
}

void updateDisplay(int percent, int raw) {
  display.setRotation(1);  // landscape (296 wide x 128 tall)
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // --- Title ---
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(8, 18);
    display.print("Garden Monitor");

    // Divider line
    display.drawFastHLine(0, 24, display.width(), GxEPD_BLACK);

    // --- Moisture bar background ---
    int barX = 8, barY = 32, barW = display.width() - 16, barH = 30;
    display.drawRect(barX, barY, barW, barH, GxEPD_BLACK);

    // Filled portion
    int fillW = map(percent, 0, 100, 0, barW - 2);
    display.fillRect(barX + 1, barY + 1, fillW, barH - 2, GxEPD_BLACK);

    // Percentage label (white inside filled area if wide enough, else black)
    String pctStr = String(percent) + "%";
    display.setFont(&FreeMonoBold9pt7b);
    int16_t tx, ty; uint16_t tw, th;
    display.getTextBounds(pctStr, 0, 0, &tx, &ty, &tw, &th);
    int labelX = barX + (barW - tw) / 2;
    int labelY = barY + barH / 2 + th / 2 - 1;

    // Draw text twice: white (clipped to filled area), black (clipped to empty)
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(labelX, labelY);
    display.print(pctStr);
    // Redraw in black over the unfilled section by drawing on top
    // (GxEPD2 is 1-bit so white-on-black already handles this automatically)

    // --- Status label ---
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.setCursor(8, 90);
    if (percent < 30) {
      display.print("Status: DRY  - needs water");
    } else if (percent < 70) {
      display.print("Status: OK   - good moisture");
    } else {
      display.print("Status: WET  - no water needed");
    }

    // --- Raw value + IP ---
    display.setFont(&FreeMono9pt7b);
    display.setCursor(8, 112);
    display.print("Raw: ");
    display.print(raw);
    display.print("   ");
    display.print(WiFi.localIP());

  } while (display.nextPage());
}

// -----------------------------------------------------------------------

void handleRoot() {
  int raw, percent;
  percent = readMoisture(&raw);

  String page = "<html><head>";
  page += "<meta http-equiv='refresh' content='5'>";
  page += "<style>body{font-family:Arial;text-align:center;padding-top:40px;}";
  page += "h1{font-size:36px;}p{font-size:28px;}</style></head><body>";
  page += "<h1>Garden Monitor</h1>";
  page += "<p>Moisture: " + String(percent) + "%</p>";
  page += "<p>Raw: " + String(raw) + "</p>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  // Init e-paper (uses default SPI2: CLK=GPIO4, MOSI=GPIO6)
  display.init(115200);
  display.setRotation(1);

  // Splash: connecting screen
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 30);
    display.print("Garden Monitor");
    display.setCursor(8, 55);
    display.print("Connecting to WiFi...");
  } while (display.nextPage());

  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  // Show IP on display
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 25);
    display.print("Garden Monitor");
    display.setCursor(8, 50);
    display.print("Connected!");
    display.setCursor(8, 75);
    display.print(WiFi.localIP().toString());
  } while (display.nextPage());

  server.on("/", handleRoot);
  server.begin();

  // Initial sensor read + display update
  int raw, percent;
  percent = readMoisture(&raw);
  updateDisplay(percent, raw);
  lastDisplayUpdate = millis();
}

void loop() {
  server.handleClient();

  // Refresh e-paper every 5 minutes
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL_MS) {
    int raw, percent;
    percent = readMoisture(&raw);
    updateDisplay(percent, raw);
    lastDisplayUpdate = millis();
  }
}
