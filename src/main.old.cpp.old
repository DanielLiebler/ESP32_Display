#include <Arduino.h>

#include <WiFi.h>												//NPT					mDNS	              WebserverAsync
#include <ESPmDNS.h>										//						mDNS
#include <WiFiClient.h>									//						mDNS
#include <NTPClient.h>									//NPT
#include <WiFiUdp.h>										//NPT
#include <FS.h>													//									              WebserverAsync
#include <AsyncTCP.h>										//									              WebserverAsync
#include <ESPAsyncWebServer.h>					//									              WebserverAsync
#include <Preferences.h>                //                  Preferences
#include "esp32_digital_led_lib.h"			//		WS2812

#include "pixelDatatypes.h"

digit_t digits[10];
digit_t digits_small[10];

#define LOG
//#define LOG_TIME
//#define LOG_PARAMS
#define PARSE_PAGE

#ifdef PARSE_PAGE
  artPage_t parsingPage;
#endif

#ifdef LOG
  #define logLine(x) if(Serial.availableForWrite()>0) Serial.print(x);
#else
  #define logLine(x) ;
#endif

#define NTP_OFFSET  1  * 60 * 60 // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "de.pool.ntp.org"

#define MODE_CLOCK_1 1
#define MODE_CLOCK_2 2
#define MODE_CLOCK_3 3

artPage_t fuckYouList[] = {(artPage_t){0x21327A2Ful, 0xA271A0Aul, 0x1A0A210Eul, 0x327B61ul}, (artPage_t){0x4A442F51ul, 0x28444428ul, 0x4284444ul, 0x476F44ul}};
artBook_t fuckYou = {.numPages=2, .pages=fuckYouList};

// Replace with your network credentials
const char* ssid     = "Highway24";
const char* password = "grandwind673AC";
const int INTEGRATED_LED =  2;
const int LEDS_PIN =  16;
const int LEDCOUNT = 105;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

Preferences preferences;
bool dst;
byte displayMode;
byte brightness;
pixelColor_t color1;
pixelColor_t color2;
pixelColor_t color3;
pixelColor_t color4;
bool colorFullEn = false;
pixelColor_t colorFull;

pixelColor_t myColor1;
pixelColor_t myColor2;
pixelColor_t myColor3;
pixelColor_t myColor4;
    
bool dst_DEF = true;
uint8_t displayMode_DEF = MODE_CLOCK_2;
uint8_t brightness_DEF = 10;
pixelColor_t color1_DEF = pixelFromRGB(255, 51, 26);
pixelColor_t color2_DEF = pixelFromRGB(128, 0, 0);
pixelColor_t color3_DEF = pixelFromRGB(0, 255, 0);
pixelColor_t color4_DEF = pixelFromRGB(0, 0, 255);
pixelColor_t colorFull_DEF = pixelFromRGB(0, 0, 255);

const char HTML[] PROGMEM = "<!DOCTYPE HTML><html><link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css\" integrity=\"sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB\" crossorigin=\"anonymous\"><head><title>Control</title><link rel=\"shortcut icon\" href=\"https://upload.wikimedia.org/wikipedia/commons/thumb/8/83/Label-clock.svg/240px-Label-clock.svg.png\" type=\"image/png\"><link rel=\"shortcut icon\" href=\"https://upload.wikimedia.org/wikipedia/commons/thumb/8/83/Label-clock.svg/240px-Label-clock.svg.png\" type=\"image/png\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><div class=\"card\"><div class=\"card-header\"> <h1>Controls</h1> </div><div class=\"card-body\"><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">Brightness</span> </div><input type=\"range\" class=\"form-control custom-range\" id=\"brightness\" min=\"0\" max=\"255\" value=\"%BRIGHTNESS_VALUE%\"/> <div class=\"input-group-append\"><button id=\"setBrightness\" class=\"btn btn-secondary\" type=\"button\" onclick='$.get(\"/\",{brightness: $(\"#brightness\").val()});'>SET</button> </div></div><div class=\"btn-group dropright\"> <button type=\"button\" class=\"btn btn-secondary dropdown-toggle\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\"> Mode </button> <div class=\"dropdown-menu\"> <button id=\"dropdownMode1\" class=\"dropdown-item%DISPLAYMODE_1%\" type=\"button\" onclick='$.get(\"/\",{displayMode: 1}); $(\"#dropdownMode1\").addClass(\"active\"); $(\"#dropdownMode2\").removeClass(\"active\"); $(\"#dropdownMode3\").removeClass(\"active\");'>Clock Mode 1</button> <button id=\"dropdownMode2\" class=\"dropdown-item%DISPLAYMODE_2%\" type=\"button\" onclick='$.get(\"/\",{displayMode: 2}); $(\"#dropdownMode2\").addClass(\"active\"); $(\"#dropdownMode1\").removeClass(\"active\"); $(\"#dropdownMode3\").removeClass(\"active\");'>Clock Mode 2</button> <button id=\"dropdownMode3\" class=\"dropdown-item%DISPLAYMODE_3%\" type=\"button\" onclick='$.get(\"/\",{displayMode: 3}); $(\"#dropdownMode3\").addClass(\"active\"); $(\"#dropdownMode1\").removeClass(\"active\"); $(\"#dropdownMode2\").removeClass(\"active\");'>Clock Mode 3</button> </div></div><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">LED #1</span> </div><button type=\"button\" class=\"btn btn-secondary form-control\" onclick='$.get(\"/\",{intergratedLed: \"true\"});'>ON</button> <button type=\"button\" class=\"btn btn-secondary form-control\" onclick='$.get(\"/\",{intergratedLed: \"false\"});'>OFF</button> </div><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">Full Color</span> </div><button type=\"button\" class=\"btn btn-success form-control\" onclick='$.get(\"/\",{fullColor: \"true\", fullColorR: 0, fullColorG: 255, fullColorB: 0});'>GREEN</button> <button type=\"button\" class=\"btn btn-danger form-control\" onclick='$.get(\"/\",{fullColor: \"true\", fullColorR: 255, fullColorG: 0, fullColorB: 0});'>RED</button> <button type=\"button\" class=\"btn btn-secondary form-control\" onclick='$.get(\"/\",{fullColor: \"false\"});'>OFF</button> </div></div></div><script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js\" integrity=\"sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49\" crossorigin=\"anonymous\"></script><script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/js/bootstrap.min.js\" integrity=\"sha384-smHYKdLADwkXOn1EmN1qk/HfnUcbVRZyYmZ4qpPea6sjB/pTJ0euyQp0Mk8ck+5T\" crossorigin=\"anonymous\"></script></html>";

AsyncWebServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;
strand_t pStrand = {.rmtChannel = 0, .gpioNum = LEDS_PIN, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = LEDCOUNT, .pixels = nullptr, ._stateVars = nullptr};
myTime_t currentTime = {.hours = 0, .minutes = 0, .seconds = 0};

void displayPage(bool binaryData[], pixelColor_t color) {
  digitalLeds_resetPixels(&pStrand);
  for (int i = 0; i < 105; i++) {
    if (binaryData[i]) pStrand.pixels[i] = color;
  }
  digitalLeds_updatePixels(&pStrand);
}

void toBool(artPage_t page, bool* binaryData) {
  //bool binaryData[105];
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 7; k++) {
        if (i%2 == 0) {
          binaryData[i*21 + j * 7 + k] = (page.data[3*i + j] >> k)%2;
        } else {
          binaryData[i*21 + 20 - j * 7 - k] = (page.data[3*i + j] >> k)%2;
        }
      }
    }
  }
  //return binaryData;
}

artPage_t parsePage(bool data[][21]) {
  artPage_t binaryData;
  binaryData.data[15] = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      binaryData.data[3*i + j] = 0;
      for (int k = 0; k < 7; k++){
        binaryData.data[3*i + j] += data[i][j*7+k] << k;
      }
    }
  }
  return binaryData;
}

void showArtBook(artBook_t artBook, int dly, pixelColor_t color) {
  bool binaryData[105];
  for (int i = 0; i < artBook.numPages; i++) {
    toBool(artBook.pages[i], binaryData);
    displayPage(binaryData, color);
    delay(dly);
  }
}

pixelColor_t calcColor(uint8_t bright, pixelColor_t color) {
  return pixelFromRGB((((uint16_t)color.r)*bright)/255, (((uint16_t)color.g)*bright)/255, (((uint16_t)color.b)*bright)/255);
}

void updateColorFull() {
  if (colorFullEn) {
    flushColor(colorFull);
  }  
}

void updateColors() {
  myColor1 = calcColor(brightness, color1);
  myColor2 = calcColor(brightness, color2);
  myColor3 = calcColor(brightness, color3);
  myColor4 = calcColor(brightness, color4);  
}

void setDst(bool data) {
  preferences.begin("clock", false);
  dst = data;
  preferences.putBool("dst", data);
  preferences.end();
  if (dst) {
    timeClient.setTimeOffset(2*NTP_OFFSET);  
  } else {
    timeClient.setTimeOffset(1*NTP_OFFSET);
  }
}

void setDisplayMode(uint8_t data) {
  preferences.begin("clock", false);
  displayMode = data;
  preferences.putUChar("displayMode", data);
  preferences.end();
}

void set_Brightness(uint8_t data) {
  preferences.begin("clock", false);
  brightness = data;
  preferences.putUChar("brightness", data);
  preferences.end();
  updateColors();
}

void setColor1(pixelColor_t data) {
  preferences.begin("clock", false);
  color1 = data;
  preferences.putUInt("color1", data.num);
  preferences.end();
  updateColors();
}

void setColor2(pixelColor_t data) {
  preferences.begin("clock", false);
  color2 = data;
  preferences.putUInt("color2", data.num);
  preferences.end();
  updateColors();
}

void setColor3(pixelColor_t data) {
  preferences.begin("clock", false);
  color3 = data;
  preferences.putUInt("color3", data.num);
  preferences.end();
  updateColors();
}

void setColor4(pixelColor_t data) {
  preferences.begin("clock", false);
  color4 = data;
  preferences.putUInt("color4", data.num);
  preferences.end();
  updateColors();
}

void setColorFull(pixelColor_t data) {
  preferences.begin("clock", false);
  colorFull = data;
  preferences.putUInt("colorFull", data.num);
  preferences.end();
  updateColorFull();
}

void loadPreferences() {
  preferences.begin("clock", false);

  dst = preferences.getBool("dst", dst_DEF);
  if (dst) {
    timeClient.setTimeOffset(2*NTP_OFFSET);  
  } else {
    timeClient.setTimeOffset(1*NTP_OFFSET);
  }
  displayMode = preferences.getUChar("displayMode", displayMode_DEF);
  brightness = preferences.getUChar("brightness", brightness_DEF);
  color1.num = preferences.getUInt("color1", color1_DEF.num);
  color2.num = preferences.getUInt("color2", color2_DEF.num);
  color3.num = preferences.getUInt("color3", color3_DEF.num);
  color4.num = preferences.getUInt("color4", color4_DEF.num);
  updateColors();
  colorFull.num = preferences.getUInt("colorFull", colorFull_DEF.num);

  preferences.end();
}

void setupLeds() {
  pinMode (LEDS_PIN, OUTPUT);
  digitalWrite (LEDS_PIN, LOW);

  if (digitalLeds_initStrands(&pStrand, 1)) {
    logLine("Init FAILURE: halting\n");
    while (true) {};
  }
  
  digitalLeds_resetPixels(&pStrand);

  digits[0] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[1] = {.pixelCount = 6 , .pixels = {{.x = 3,.y = 0}, {.x = 2,.y = 1}, {.x = 3,.y = 1}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 3,.y = 4}}};
  digits[2] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[3] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[4] = {.pixelCount = 9 , .pixels = {{.x = 0,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 1,.y = 3}, {.x = 2,.y = 3}, {.x = 3,.y = 3}, {.x = 2,.y = 4}}};
  digits[5] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[6] = {.pixelCount = 15, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[7] = {.pixelCount = 9 , .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 3,.y = 4}}};
  digits[8] = {.pixelCount = 16, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[9] = {.pixelCount = 15, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};

  digits_small[0] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[1] = {.pixelCount =  5, .pixels = {{.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[2] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[3] = {.pixelCount = 10, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[4] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[5] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, }};
  digits_small[6] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[7] = {.pixelCount =  8, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[8] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[9] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
}

void checkParams(AsyncWebServerRequest *request) {
  if(request->hasParam("dst")) {
    AsyncWebParameter* p = request->getParam("dst");
    setDst(p->value().equalsIgnoreCase("true"));
    printTime(8);
  }
  if(request->hasParam("displayMode")) {
    AsyncWebParameter* p = request->getParam("displayMode");
    setDisplayMode(p->value().toInt());
    printTime(8);
  }
  if(request->hasParam("brightness")) {
    AsyncWebParameter* p = request->getParam("brightness");
    set_Brightness(p->value().toInt());
    printTime(8);
  }
  if(request->hasParam("color1R") && request->hasParam("color1G") && request->hasParam("color1B")) {
    AsyncWebParameter* r = request->getParam("color1R");
    AsyncWebParameter* g = request->getParam("color1G");
    AsyncWebParameter* b = request->getParam("color1B");
    setColor1(pixelFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime(8);
  }
  if(request->hasParam("color2R") && request->hasParam("color2G") && request->hasParam("color2B")) {
    AsyncWebParameter* r = request->getParam("color2R");
    AsyncWebParameter* g = request->getParam("color2G");
    AsyncWebParameter* b = request->getParam("color2B");
    setColor2(pixelFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime(8);
  }
  if(request->hasParam("color3R") && request->hasParam("color3G") && request->hasParam("color3B")) {
    AsyncWebParameter* r = request->getParam("color3R");
    AsyncWebParameter* g = request->getParam("color3G");
    AsyncWebParameter* b = request->getParam("color3B");
    setColor3(pixelFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime(8);
  }
  if(request->hasParam("color4R") && request->hasParam("color4G") && request->hasParam("color4B")) {
    AsyncWebParameter* r = request->getParam("color4R");
    AsyncWebParameter* g = request->getParam("color4G");
    AsyncWebParameter* b = request->getParam("color4B");
    setColor4(pixelFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime(8);
  }
  if(request->hasParam("intergratedLed")) {
    AsyncWebParameter* p = request->getParam("intergratedLed");
    if(p->value().equalsIgnoreCase("true")) {
      digitalWrite(INTEGRATED_LED, HIGH);
      showArtBook(fuckYou, 800, color4);
      printTime(8);
    } else {
      digitalWrite(INTEGRATED_LED, LOW);  
    }
  }
  if(request->hasParam("fullColorR") && request->hasParam("fullColorG") && request->hasParam("fullColorB")) {
    AsyncWebParameter* r = request->getParam("fullColorR");
    AsyncWebParameter* g = request->getParam("fullColorG");
    AsyncWebParameter* b = request->getParam("fullColorB");
    setColorFull(pixelFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime(8);
  }
  if(request->hasParam("fullColor")) {
    AsyncWebParameter* p = request->getParam("fullColor");
    colorFullEn = (p->value().equalsIgnoreCase("true"));
    updateColorFull();
  }
}

String processor(const String& var) {
  //%HELLO_FROM_TEMPLATE%
  if(var == "BRIGHTNESS_VALUE")
    return String(brightness); //F("20");
  if(var == "DISPLAYMODE_1" && displayMode == MODE_CLOCK_1)
    return F(" active");
  if(var == "DISPLAYMODE_2" && displayMode == MODE_CLOCK_2)
    return F(" active");
  if(var == "DISPLAYMODE_3" && displayMode == MODE_CLOCK_3)
    return F(" active");
  return String();
}

void setupAsyncServer() {
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		int paramsNr = request->params();
		if (paramsNr > 0) {
      #ifdef LOG_PARAMS
			  for(int i=0;i<paramsNr;i++){
     	  	AsyncWebParameter* p = request->getParam(i);
 
     	  	Serial.print("Param name: ");
     	  	Serial.println(p->name());
 
     	  	Serial.print("Param value: ");
     	  	Serial.println(p->value());
 
     	  	Serial.println("------");
			  }
      #endif
      checkParams(request);
		}
		request->send_P(200, "text/html", HTML, processor);
	});
}

void setupWifiServer() {
  // We start by connecting to a WiFi network
  logLine("\n");
  logLine("\n");
  logLine("Connecting to ");
  logLine(ssid);
  logLine("\n");

  WiFi.begin(ssid, password);
  
  //digitalWrite(INTEGRATED_LED, HIGH);
  
  // attempt to connect to Wifi network:
  while(WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    logLine(".");
  }
  logLine("\n");
  logLine("WiFi connected\n");
  logLine("IP address: \n");
  logLine(WiFi.localIP());
  logLine("\n");

  if (!MDNS.begin("esp32")) {
    logLine("Error setting up MDNS responder!\n");
    while(1) {
      delay(1000);
    }
  }
  logLine("mDNS responder started\n");
  
  setupAsyncServer();
  server.begin();
  
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  #ifdef PARSE_PAGE

    bool vars[5][21] = { {1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}, 
                         {0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
                         {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
                         {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
                         {0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1}};

    parsingPage = parsePage(vars);
    Serial.printf("Parsed page 0x%X, 0x%X, 0x%X, 0x%X", parsingPage.num[0], parsingPage.num[1], parsingPage.num[2], parsingPage.num[3]);
    Serial.println();
  #endif
}

void flushColor(pixelColor_t color) {
  for (int i = 0; i < LEDCOUNT; i++) {
    pStrand.pixels[i] = color;
  }  
  digitalLeds_updatePixels(&pStrand);
}

void setPixel(int x, int y, pixelColor_t color) {
  if (x > 20) return;
  if (y > 4) return;
  if (x < 0) return;
  if (y < 0) return;
  int pos = 0;
  if (y%2 == 0) {
    pos = 21*y + x;  
  } else {
    pos = 21*(y+1) - (x+1);
  }
  pStrand.pixels[pos] = color;
}

void printDigit(int digit, int xOffset, pixelColor_t color, digit_t font[]) {
  if (xOffset > 20) return;
  if (xOffset < -3) return;
  for (int i = 0; i < font[digit].pixelCount; i++) {
    setPixel(xOffset + font[digit].pixels[i].x, font[digit].pixels[i].y, color);
  }
}

void printTime(int change) {
  if (colorFullEn) return;
  if (displayMode == MODE_CLOCK_1 && change > 1) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, myColor1, digits);
    printDigit(currentTime.hours % 10, 5, myColor1, digits);
    if (currentTime.seconds%2 == 0) {
      setPixel(10, 1, myColor2);
      setPixel(10, 3, myColor2);
    }
    printDigit(currentTime.minutes / 10, 12, myColor1, digits);
    printDigit(currentTime.minutes % 10, 17, myColor1, digits);
    digitalLeds_updatePixels(&pStrand);
  }else if (displayMode == MODE_CLOCK_2 && change > 2) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, myColor1, digits);
    printDigit(currentTime.hours % 10, 5, myColor1, digits);
    setPixel(10, 1, myColor2);
    setPixel(10, 3, myColor2);
    printDigit(currentTime.minutes / 10, 12, myColor1, digits);
    printDigit(currentTime.minutes % 10, 17, myColor1, digits);
    digitalLeds_updatePixels(&pStrand);
  }else if (displayMode == MODE_CLOCK_3 && change > 1) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, myColor1, digits_small);
    printDigit(currentTime.hours % 10, 4, myColor1, digits_small);
    printDigit(currentTime.minutes / 10, 7, myColor2, digits_small);
    printDigit(currentTime.minutes % 10, 11, myColor2, digits_small);
    printDigit(currentTime.seconds / 10, 14, myColor3, digits_small);
    printDigit(currentTime.seconds % 10, 18, myColor3, digits_small);
    digitalLeds_updatePixels(&pStrand);
  }

  
}

void updateTime(){
  timeClient.update();
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  int change = 0;
  if (hours != currentTime.hours){
    change = 4;
  } else if (minutes != currentTime.minutes){
    change = 3;
  } else if (seconds != currentTime.seconds){
    change = 2;
  }
  
  if(change > 0) {
    currentTime.hours = hours;
    currentTime.minutes = minutes;
    currentTime.seconds = seconds;
    String formattedTime = timeClient.getFormattedTime();
    #ifdef LOG_TIME
      logLine(currentTime.hours);
      logLine(":");
      logLine(currentTime.minutes);
      logLine(":");
      logLine(currentTime.seconds);
      logLine(" -- ");
      logLine(formattedTime);
      logLine("\n");
    #endif
    printTime(change);
  }
}

void setup() {
  timeClient.begin();
  // initialize the LEDs pins as an output:
  pinMode(INTEGRATED_LED, OUTPUT);

  #ifdef LOG
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    //while(!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    //}
  #endif
  
  loadPreferences();
  setupLeds();
  setupWifiServer();
  digitalWrite(INTEGRATED_LED, LOW);
}

void loop() {
  updateTime();
  delay(100);
}