#include <WiFi.h>												//NPT					mDNS	WebserverAsync	WebserverSync
#include <ESPmDNS.h>										//						mDNS
#include <WiFiClient.h>									//						mDNS
#include <NTPClient.h>									//NPT
#include <WiFiUdp.h>										//NPT
#include <FS.h>													//									WebserverAsync
#include <AsyncTCP.h>										//									WebserverAsync
#include <ESPAsyncWebServer.h>					//									WebserverAsync
#include "esp32_digital_led_lib.h"			//		WS2812

#include "pixelDatatypes.h"

digit_t digits[10];
digit_t digits_small[10];

#define LOG
//#define LOG_TIME

#ifdef LOG
  #define logLine(x) if(Serial.availableForWrite()>0) Serial.print(x);
#else
  #define logLine(x) ;
#endif

#define NTP_OFFSET  1  * 60 * 60 // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "de.pool.ntp.org"
#define DST_PIN 4

// Replace with your network credentials
const char* ssid     = "Highway24";
const char* password = "grandwind673AC";
const int INTEGRATED_LED =  2;
const int LEDS_PIN =  16;
const int LEDCOUNT = 105;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

const char HTML[] PROGMEM = "<!DOCTYPE HTML><html><link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css\" integrity=\"sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB\" crossorigin=\"anonymous\"><head><title>Control</title><link rel=\"shortcut icon\" href=\"https://upload.wikimedia.org/wikipedia/commons/thumb/8/83/Label-clock.svg/240px-Label-clock.svg.png\" type=\"image/png\"><link rel=\"shortcut icon\" href=\"https://upload.wikimedia.org/wikipedia/commons/thumb/8/83/Label-clock.svg/240px-Label-clock.svg.png\" type=\"image/png\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><div class=\"card\"><div class=\"card-header\"> <h1>Controls</h1> </div><div class=\"card-body\"><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">Brightness</span> </div><input type=\"range\" class=\"form-control custom-range\" id=\"brightness\" min=\"0\" max=\"255\" value=\"10\" onchange=\"document.getElementById('setBrightness').setAttribute('href', 'bright' + this.value);\"/> <div class=\"input-group-append\"><button id=\"setBrightness\" class=\"btn btn-secondary\" type=\"button\">SET</button> </div></div><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">LED #1</span> </div><button type=\"button\" class=\"btn btn-secondary form-control\">ON</button> <button type=\"button\" class=\"btn btn-secondary form-control\">OFF</button> </div><div class=\"input-group mb-3\"> <div class=\"input-group-prepend\"> <span class=\"input-group-text\">Full Color</span> </div><button type=\"button\" class=\"btn btn-success form-control\">GREEN</button> <button type=\"button\" class=\"btn btn-danger form-control\">RED</button> </div></div></div><i class=\"fas fa-clock\"></i><script src=\"https://code.jquery.com/jquery-3.3.1.slim.min.js\" integrity=\"sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo\" crossorigin=\"anonymous\"></script><script src=\"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js\" integrity=\"sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49\" crossorigin=\"anonymous\"></script><script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/js/bootstrap.min.js\" integrity=\"sha384-smHYKdLADwkXOn1EmN1qk/HfnUcbVRZyYmZ4qpPea6sjB/pTJ0euyQp0Mk8ck+5T\" crossorigin=\"anonymous\"></script></html>";

//WiFiServer server(80);
AsyncWebServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;
strand_t pStrand = {.rmtChannel = 0, .gpioNum = LEDS_PIN, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = LEDCOUNT, .pixels = nullptr, ._stateVars = nullptr};
myTime_t currentTime = {.hours = 0, .minutes = 0, .seconds = 0};
int timeFormat = 1;
pixelColor_t timeColor = pixelFromRGB(10, 2, 1);
pixelColor_t dotColor = pixelFromRGB(5, 0, 0);

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
  digits_small[2] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[3] = {.pixelCount = 10, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[4] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[5] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, }};
  digits_small[6] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[7] = {.pixelCount =  8, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[8] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[9] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
}

void setupAsyncServer() {
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		int paramsNr = request->params();
		if (paramsNr > 0) {
			for(int i=0;i<paramsNr;i++){
     		AsyncWebParameter* p = request->getParam(i);
 
     		Serial.print("Param name: ");
     		Serial.println(p->name());
 
     		Serial.print("Param value: ");
     		Serial.println(p->value());
 
     		Serial.println("------");
			}
		}
		request->send(200, "text/html", HTML);
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

void printDigit(int digit, int xOffset, pixelColor_t color) {
  if (xOffset > 20) return;
  if (xOffset < -3) return;
  for (int i = 0; i < digits[digit].pixelCount; i++) {
    setPixel(xOffset + digits[digit].pixels[i].x, digits[digit].pixels[i].y, color);
  }
}

void printTime(int change) {
  if (timeFormat == 0) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, timeColor);
    printDigit(currentTime.hours % 10, 5, timeColor);
    if (currentTime.seconds%2 == 0) {
      setPixel(10, 1, dotColor);
      setPixel(10, 3, dotColor);
    }
    printDigit(currentTime.minutes / 10, 12, timeColor);
    printDigit(currentTime.minutes % 10, 17, timeColor);
    digitalLeds_updatePixels(&pStrand);
  }else if (timeFormat == 1 && change > 2) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, timeColor);
    printDigit(currentTime.hours % 10, 5, timeColor);
    setPixel(10, 1, dotColor);
    setPixel(10, 3, dotColor);
    printDigit(currentTime.minutes / 10, 12, timeColor);
    printDigit(currentTime.minutes % 10, 17, timeColor);
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

  pinMode(DST_PIN, INPUT_PULLUP);
  if(digitalRead(DST_PIN) != HIGH) {
    logLine("Sommerzeit, GMT+2 \n");
    timeClient.setTimeOffset(2*NTP_OFFSET);
  }else{
    logLine("Winterzeit, GMT+1\n");
  }
  
  setupLeds();
  setupWifiServer();
  digitalWrite(INTEGRATED_LED, LOW);
}

void loop() {
  updateTime();
  delay(100);
}
