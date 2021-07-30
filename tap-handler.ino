//
// tap-handler.ino
// Naookie Sato
//
// Created by Naookie Sato on 05/06/2021
// Copyright © 2021 Naookie Sato. All rights reserved.
//

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "SetHtml.h"
#include <SSD1306.h>

// standard tap rate of one gallon per minute
double FLOW_RATE_SEC = 128.0 / 60.0;
double FLOW_RATE_MSEC = FLOW_RATE_SEC / 1000;

struct Pour
{
  double msec_ = 0;
  double floz_ = 0;
};

struct TapHandle
{
  TapHandle(int pin) : pin_(pin) {}

  String brewery_ = "Unknown Brewery";
  String name_ = "Unknown Beer";
  double abv_ = 0;
  int ibu_ = 0;
  double maxFloz_ = 5 * 128;
  double fill_ = 100;
  double pin_;
  double flowRate_ = FLOW_RATE_MSEC;
  Pour lastPour_;
};

TapHandle handle1(D6);
TapHandle handle2(D5);

SSD1306 display1(0x3C, D2, D1);
SSD1306 display2(0x3D, D2, D1);

const char AP_NAME[] = "tap-handler";
const char WiFiAPPSK[] = "tap-handler";

int wifiStatus;
IPAddress ip(172,16,168,1);      // this node's soft ap ip address
IPAddress gateway(172,16,168,1); // this node's soft ap gateway
IPAddress subnet(255,255,255,0); // this node's soft ap subnet mask
ESP8266WebServer server(80);

std::function<void()> handleClient = []() {
  MDNS.update();
  server.handleClient();
};

void updateDisplay(SSD1306& d, TapHandle& h)
{
  int brewery_y = 0;
  int name_y = brewery_y + 11;
  int abv_y = name_y + 16;
  int ibu_y = abv_y + 10;
  int fill_y = 49;
  int bar_y = 50;

  d.clear();
  d.setFont(ArialMT_Plain_10);
  d.drawString(0, brewery_y, h.brewery_);
  d.setFont(ArialMT_Plain_16);
  d.drawString(0, name_y, h.name_);
  d.setFont(ArialMT_Plain_10);
  d.drawString(0, abv_y, String(h.abv_) + "\% ABV");
  d.drawString(0, ibu_y, String(h.ibu_) + " IBUs");
  d.drawString(0, fill_y, String((uint8_t)h.fill_) + "\%");
  d.drawRect(27, bar_y, 100, 11);
  d.fillRect(27, bar_y, (uint8_t)h.fill_, 11);
  d.display();
}
 
void updateDisplayWhilePulled(SSD1306& d, TapHandle& h)
{
  int pour_y = 0;
  int fill_y = 30;
  int bar_y = 50;

  d.clear();
  d.setFont(ArialMT_Plain_16);
  d.drawString(0, pour_y, String(h.lastPour_.floz_) + "oz");
  d.drawString(64, pour_y, String(h.lastPour_.msec_/1000) + "s");
  d.drawString(0, fill_y, String(h.fill_) + "\%");
  d.drawRect(13, bar_y, 100, 11);
  d.fillRect(13, bar_y, (uint8_t)h.fill_, 11);
  d.display();
}
  
void updateDisplaysWhilePulled()
{
  updateDisplayWhilePulled(display1, handle1);
  updateDisplayWhilePulled(display2, handle2);
};

void updateDisplays()
{
  updateDisplay(display1, handle1);
  updateDisplay(display2, handle2);
};

void setupWiFi()
{
  Serial.print("This device's MAC address is: ");
  Serial.println(WiFi.softAPmacAddress());

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(AP_NAME, WiFiAPPSK, 6, 0);
  Serial.print("This AP's IP address is: ");
  Serial.println(WiFi.softAPIP());  
  if (MDNS.begin("tap-handler"))
  {
    Serial.println("MDNS tap-handler started");
  }
}

void initHardware()
{
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(handle1.pin_, INPUT); 
  pinMode(handle2.pin_, INPUT); 
  digitalWrite(LED_BUILTIN, HIGH);//on Lolin ESP8266 v3 dev boards, the led is active low
  delay(1000);
}

void handleSet()
{
  String message = "Setting tap handles:\n";
  if (server.hasArg("brewery-1") && server.arg("brewery-1") != "")
  {
    handle1.brewery_ = server.arg("brewery-1");
    message += "    brewery-1: " + server.arg("brewery-1") + "\n";
  }
  if (server.hasArg("brewery-2") && server.arg("brewery-2") != "")
  {
    handle2.brewery_ = server.arg("brewery-2");
    message += "    brewery-2: " + server.arg("brewery-2") + "\n";
  }
  if (server.hasArg("brew-name-1") && server.arg("brew-name-1") != "")
  {
    handle1.name_ = server.arg("brew-name-1");
    message += "    brew-name-1: " + server.arg("brew-name-1") + "\n";
  }
  if (server.hasArg("brew-name-2") && server.arg("brew-name-2") != "")
  {
    handle2.name_ = server.arg("brew-name-2");
    message += "    brew-name-2: " + server.arg("brew-name-2") + "\n";
  }
  if (server.hasArg("abv-1") && server.arg("abv-1") != "")
  {
    handle1.abv_ = server.arg("abv-1").toDouble();
    message += "    abv-1: " + server.arg("abv-1") + "\n";
  }
  if (server.hasArg("abv-2") && server.arg("abv-2") != "")
  {
    handle2.abv_ = server.arg("abv-2").toDouble();
    message += "    abv-2: " + server.arg("abv-2") + "\n";
  }
  if (server.hasArg("fill-1") && server.arg("fill-1") != "")
  {
    handle1.fill_ = server.arg("fill-1").toDouble();
    message += "    fill-1: " + server.arg("fill-1") + "\n";
  }
  if (server.hasArg("fill-2") && server.arg("fill-2") != "")
  {
    handle2.fill_ = server.arg("fill-2").toDouble();
    message += "    fill-2: " + server.arg("fill-2") + "\n";
  }
  if (server.hasArg("ibu-1") && server.arg("ibu-1") != "")
  {
    handle1.ibu_ = server.arg("ibu-1").toDouble();
    message += "    ibu-1: " + server.arg("ibu-1") + "\n";
  }
  if (server.hasArg("ibu-2") && server.arg("ibu-2") != "")
  {
    handle2.ibu_ = server.arg("ibu-2").toDouble();
    message += "    ibu-2: " + server.arg("ibu-2") + "\n";
  }

  server.send(200, "text/html", setHtml);
  Serial.println(message);
}

void initDisplays()
{
  display1.init();
  //display1.flipScreenVertically();
  display1.clear();
  display1.display();

  display2.init();
  //display2.flipScreenVertically();
  display2.clear();
  display2.display();
}

void setup()
{
  initHardware();
  initDisplays();
  setupWiFi();
  server.on("/", handleSet);
  server.on("/set", handleSet);
  server.begin();
}

bool updateHandle(int t, TapHandle& h, SSD1306& d)
{
    if (digitalRead(h.pin_) == HIGH)
    {
      auto delayFlow = t * h.flowRate_;
      h.lastPour_.msec_ += t;
      h.lastPour_.floz_ += delayFlow;
      h.fill_ -= 100 * delayFlow / h.maxFloz_;
      updateDisplayWhilePulled(d, h);
      return true;
    }
    updateDisplay(d, h);
    return false;
}

void updateHandles()
{
  bool h1 = digitalRead(handle1.pin_) == HIGH;
  bool h2 = digitalRead(handle2.pin_) == HIGH;
  int delayTime = 100;
  while (h1 || h2)
  {
    delay(delayTime);
    // Actuall etime is greater due to display updates
    double delayConstant = 1.20;
    h1 = updateHandle(delayTime*delayConstant, handle1, display1);
    h2 = updateHandle(delayTime*delayConstant, handle2, display2);
    if (!h1)
    {
      handle1.lastPour_ = Pour();
    }
    if (!h2)
    {
      handle2.lastPour_ = Pour();
    }
  }
}

void loop()
{
  updateHandles();
  handleClient();
  updateDisplays();
  delay(100);
}
