#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "esp32_digital_led_lib.h"

// Replace with your network credentials
const char* ssid     = "Highway24";
const char* password = "grandwind673AC";
const int INTEGRATED_LED =  2;
const int LEDS_PIN =  16;
const int LEDCOUNT = 105;

WiFiServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;
strand_t pStrand = {.rmtChannel = 0, .gpioNum = LEDS_PIN, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = LEDCOUNT, .pixels = nullptr, ._stateVars = nullptr};

void setupLeds() {
  pinMode (LEDS_PIN, OUTPUT);
  digitalWrite (LEDS_PIN, LOW);

  if (digitalLeds_initStrands(&pStrand, 1)) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  
  digitalLeds_resetPixels(&pStrand);
}

void setupWifiServer() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // attempt to connect to Wifi network:
  while(WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  server.begin();
  
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void buildWebpage(WiFiClient client) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();
  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
  client.println("<h1>ESP32 - Web Server</h1>");
  client.println("<p>LED #1 <a href=\"on1\"><button>ON</button></a>&nbsp;<a href=\"off1\"><button>OFF</button></a></p>");
  client.println("<p>LED #2 <a href=\"on2\"><button>ON</button></a>&nbsp;<a href=\"off2\"><button>OFF</button></a></p>");
  client.println("</html>");
}

void checkParameters() {
  if (strstr(linebuf,"GET /on1") > 0) {
    Serial.println("LED 1 ON");
    digitalWrite(INTEGRATED_LED, HIGH);
  } else if (strstr(linebuf,"GET /off1") > 0) {
    Serial.println("LED 1 OFF");
    digitalWrite(INTEGRATED_LED, LOW);
  } else if (strstr(linebuf,"GET /on2") > 0) {
    Serial.println("LED 2 ON");
    flushColor(pixelFromRGB(0, 255, 0));
  } else if (strstr(linebuf,"GET /off2") > 0) {
    Serial.println("LED 2 OFF");
    flushColor(pixelFromRGB(255, 0, 0));
  }
}

void listenforClients() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          buildWebpage(client);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          checkParameters();
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void flushColor(pixelColor_t color) {
  for (int i = 0; i < LEDCOUNT; i++) {
    pStrand.pixels[i] = color;
  }  
  digitalLeds_updatePixels(&pStrand);
}

void setup() {
  // initialize the LEDs pins as an output:
  pinMode(INTEGRATED_LED, OUTPUT);
  
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  setupLeds();
  setupWifiServer();
}

void loop() {
  listenforClients();
}
