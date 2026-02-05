#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>

#include <DHT.h>
#include <ArduinoJson.h>

/* ================= WIFI ================= */

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

/* ================= TIME ================= */

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;       // Change for your country
const int daylightOffset_sec = 0;

/* ================= SENSOR ================= */

#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

/* ================= E-INK DISPLAY ================= */

// Change model if needed
GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display(
  GxEPD2_213_B73(5, 17, 16, 4) // CS, DC, RST, BUSY
);

/* ================= API ================= */

String apiURL = "https://api.coindesk.com/v1/bpi/currentprice.json";

/* ================= FUNCTIONS ================= */

void connectWiFi()
{
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
}

String getTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    return "No Time";
  }

  char buffer[30];
  strftime(buffer, 30, "%H:%M:%S", &timeinfo);

  return String(buffer);
}

float getOnlineData()
{
  HTTPClient http;
  http.begin(apiURL);

  int code = http.GET();

  if (code == 200)
  {
    String payload = http.getString();

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    float price = doc["bpi"]["USD"]["rate_float"];

    http.end();
    return price;
  }

  http.end();
  return -1;
}

void drawScreen(String timeStr, float temp, float hum, float online)
{
  display.setFullWindow();
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);

    display.setTextColor(GxEPD_BLACK);

    display.setTextSize(2);
    display.setCursor(10, 25);
    display.println("ESP32 E-Ink");

    display.setTextSize(2);
    display.setCursor(10, 55);
    display.print("Time: ");
    display.println(timeStr);

    display.setCursor(10, 85);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");

    display.setCursor(10, 110);
    display.print("Hum: ");
    display.print(hum);
    display.println(" %");

    display.setCursor(10, 140);
    display.print("BTC: $");

    if (online > 0)
      display.println(online);
    else
      display.println("Error");

  }
  while (display.nextPage());
}

/* ================= SETUP ================= */

void setup()
{
  Serial.begin(115200);

  display.init();

  dht.begin();

  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

/* ================= LOOP ================= */

void loop()
{
  String timeNow = getTime();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  float onlineValue = getOnlineData();

  Serial.println("Updating display...");

  drawScreen(timeNow, temperature, humidity, onlineValue);

  // Sleep 5 minutes (e-ink friendly)
  delay(300000);
}
