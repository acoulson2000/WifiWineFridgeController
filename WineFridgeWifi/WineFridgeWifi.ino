/*
 * Required libraries:
 * 
 * ArduinoJson:   https://github.com/bblanchon/ArduinoJson
 * WiFiManager:   https://github.com/tzapu/WiFiManager
 * Adafruit_GFX:  https://github.com/adafruit/Adafruit-GFX-Library
 * Adafruit_SSD1306: https://github.com/adafruit/Adafruit_SSD1306
 * SoftwareSerial:  (ESP version) https://github.com/plerup/espsoftwareserial
 * 
 * This is a sketch for the front-end controller of the wine refrigerator. 
 * It is intended to run on a Heltec "WIfi Kit 8" ESP8266 + 96x32 OLED board, but could 
 * be adapted to other ESP8266 + OLED compbinations. It provides a web interface
 * for displaying and setting the fridge temps (with the option to have button-controlled
 * settings, as well), displays the temp, and sends the setting to an Arduino/Monster
 * Moto shield combo on the backend which drive power ti the fans and peltier heat sinks. 
 * It sends the target temp to the Arduino periodically over serial2. The Arduino
 * also senses the current temp, which it sends back to this front-end periodically
 * over serial2.  Various debugging and diagnostics are printed on SERIAL.
 */
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>         //For rendering to Heltec OLED screen (uses SSD1306 chip)
#include <Adafruit_SSD1306.h>

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino


#include <DNSServer.h>            //needed for WiFiManager
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "html.h"                 //Part of project - strings for http server responses

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include "user_interface.h"

#define SERIAL2_RX 12
#define SERIAL2_TX 15
#define OLED_RESET 16 // override necessary to make OLED on Wifi Kit 8 work
#define CONFIG_PORTAL_TIMEOUT 60 // how long to stay in config mode on boot-up
// You can optionaly have buttons to control the temp setting in addition to WiFi.
// If so, set the pin numbers for the temp increase and decrease buttons here.
// (pull low when pressed)
int GPIO_TEMP_DOWN = 14;
int GPIO_TEMP_UP = 13;

//define default startup values here. Target setpoint is written to config.json in the SPIFFS
//file system after being set. After being set once, that value will override the ones here
//on boot. The current temp will be set periodically when received from the Arduino fan controller.
unsigned int fridgeTempCurrent = 60;
unsigned int fridgeTempSetpoint = 60;
char Temp_Setpoint[3] = "60";
unsigned int pwmVal = 0;
boolean ackOK = false;

Adafruit_SSD1306 display(OLED_RESET);

SoftwareSerial serial2(SERIAL2_RX, SERIAL2_TX, false, 256);

// Image byte arrays for rendering wifi available/unavailable symbols
static const uint8_t wifi_connected[] = {
0x0,0x0,0x7,0xc0,0x1f,0xf0,0x3f,0xf8,0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0xf,0xe0,0x7,0xc0,0x3,0x80,0x1,0x0,0x0,0x0
};
static const uint8_t wifi_broken[] = {
0x00, 0x10, 0x38, 0x7C, 0xFC, 0xFE, 0xFE, 0xC2, 0xFE, 0xFE, 0xFC, 0x7C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x03, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

char* CONFIG_AP = "ESPFridge";    //Set to your preference - Available to connect to when WiFiManager is in config mode

char jsonValueBuffer[3];
char stringBuffer[64];
int lastMillis = millis();

std::unique_ptr<ESP8266WebServer> server;

void handleRoot() {
  Serial.println("handle root");
  sprintf(stringBuffer, "ESPFridge Temp:%d  Target Temp:%d.", fridgeTempCurrent, fridgeTempSetpoint);
  String indexHtml = MAIN_page;
  String sTemp = String(fridgeTempCurrent, DEC);
  indexHtml.replace("{{temp}}", sTemp);
  indexHtml.replace("{{targetTemp}}", Temp_Setpoint);
  server->send(200, "text/html", indexHtml);
}

void handleSetTemp() {
  Serial.println("handle settemp");
  String pTemp = server->arg("temp");
  Serial.print("set temp=");
  Serial.println(pTemp);
  pTemp.toCharArray(Temp_Setpoint, 3);
  Serial.print("char[]=");
  Serial.println(Temp_Setpoint);
  fridgeTempSetpoint = atoi(Temp_Setpoint);
  Serial.print("send temp to controller:");
  Serial.println(fridgeTempSetpoint,DEC);
  serial2.write(fridgeTempSetpoint);
  saveConfig();
  server->send(200, "text/html", "set temp=" + String(Temp_Setpoint));
}

void handleStatus() {
  Serial.println("handle status");
  sprintf (stringBuffer, "{currentTemp=\"%d\", targetTemp=\"%d\"}", fridgeTempCurrent, fridgeTempSetpoint);
  server->send(200, "text/json", stringBuffer);
}

void handleReboot() {
  Serial.println("handle reboot");
  server->send(200, "text/plain", "Rebooting...");
  ESP.restart();
  delay(5000);  
}

void handleNotFound() {
  Serial.println("handle not found:");
  Serial.println(server->uri());
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}

  void saveConfig() {
  //save the custom parameters to FS
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["temp_setpoint"] = Temp_Setpoint;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    Serial.println();
    //end save
  }

void setup() {
  pinMode(GPIO_TEMP_DOWN, INPUT_PULLUP);
  pinMode(GPIO_TEMP_UP, INPUT_PULLUP);
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  serial2.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();
  delay(2000);
  // Clear the buffer.
  display.clearDisplay();
  display.display();
  
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          if (json.containsKey("temp_setpoint")) {
            strcpy(Temp_Setpoint, json["temp_setpoint"]);
            fridgeTempSetpoint = atoi(Temp_Setpoint);
          }

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  WiFi.hostname("ESPFridge");

  // 
  // First fire up the WiFiManager portal - this give's a user time to configure the AP
  // and credentials, if they choose.
  //
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,2,77), IPAddress(192,168,2,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  WiFiManagerParameter custom_text("<br/>Fridge Target Temperature");
  wifiManager.addParameter(&custom_text);
  WiFiManagerParameter p_target_temp("target_temp", "Target Temperature", Temp_Setpoint, 3);
  wifiManager.addParameter(&p_target_temp);

  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setConnectTimeout(60);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("STARTUP");
  display.print("Config AP:");
  display.println(CONFIG_AP);
  display.setCursor(16,20);
  display.println("wifi.urremote.com");
  display.display();

  if (!wifiManager.startConfigPortal(CONFIG_AP)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
    Serial.print("Current IP Address:"); Serial.println(WiFi.softAPIP());
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("WiFi connect fail");
    display.print("Config via: ");
    display.print(CONFIG_AP);
    display.print(" / " + WiFi.softAPIP());
    display.display();
    delay(5000);
  }

  struct softap_config conf;
  wifi_softap_get_config(&conf);

  //if you get here you have connected to the WiFi
  Serial.println("connected...yay :)");
  Serial.println((char *) conf.ssid);
  Serial.println((char *) conf.password);

  //
  // Here we fall out of the WiFiManager. If a valid access point was configured,
  // we should be connect. If connection failed or we didn't cnfigure one, we should
  // still maintain a host AP. In either case, the IP to connect to will be displayed
  // in the OLED in addition to Temp and Temp setting once we enter loop().
  //
  
  //read updated parameters
  strcpy(Temp_Setpoint, p_target_temp.getValue());
  Serial.print("New Temp setpoint is :"); Serial.println(Temp_Setpoint);

  saveConfig();
  fridgeTempSetpoint = atoi(Temp_Setpoint);

  // send to Arduino controller
  serial2.write(fridgeTempSetpoint);
  Serial.print("New Temp setpoint is :");Serial.println(Temp_Setpoint);

  // Configure out simple Web Server. A simple page is served at the root url displaying
  // temp and settings and permitting setting of target temp.
  // A status url at /status that returns both in JSON is provided for potential home autmation
  // purposes. The /reboot url will reboot this device.
  
  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
  server->on("/", handleRoot);
  server->on("/settemp", handleSetTemp);
  server->on("/status", handleStatus);
  server->on("/reboot", handleReboot);
  server->onNotFound(handleNotFound);
  server->begin();
  
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

int i = LOW;
int lastBtnTempDown = 0;
int lastBtnTempUp = 0;
int currentBtnTempDown = 0;
int currentBtnTempUp = 0;
long lastBtnMillis = 0;
boolean updateDisplay = false;

void checkForTempUpdate() {
  if (serial2.available() > 0) {
    Serial.print("Temp:");
    int ch = serial2.read();
    Serial.print(ch, DEC);
    if (ch > 32 && ch < 90) {
      fridgeTempCurrent = ch;
    }
  }   
}

void checkForAck() {
  serial2.write((byte)0);
  while (serial2.available() == 0) {
    delay(5);
  }
  Serial.print("Got 4...");
  if (serial2.read() == 'O' && serial2.read() == 'K') {
    ackOK = true;
    serial2.read();
    serial2.read();
    pwmVal = serial2.read();
    Serial.print("Got ack:");
    Serial.println(pwmVal, DEC);
  } else {
    ackOK = false;
    Serial.println("Bad data");
  }
}

void checkForButtonPress() {
  if (millis() - lastBtnMillis > 30) {
    currentBtnTempDown = digitalRead(GPIO_TEMP_DOWN);
    currentBtnTempUp = digitalRead(GPIO_TEMP_UP);
    if (lastBtnTempDown != currentBtnTempDown) {
     Serial.print("Down button change: ");Serial.println(currentBtnTempDown);
    }
    if (lastBtnTempUp != currentBtnTempUp) {
     Serial.print("Up button change: ");Serial.println(currentBtnTempUp);
    }
    if (lastBtnTempDown == 1 && currentBtnTempDown == 0 && fridgeTempSetpoint > 40) {
      fridgeTempSetpoint -= 1;
      updateDisplay = true;
    }
    if (lastBtnTempUp == 1 && currentBtnTempUp == 0 && fridgeTempSetpoint < 80) {
      fridgeTempSetpoint += 1;  
      updateDisplay = true;
    }
    lastBtnTempDown = currentBtnTempDown;
    lastBtnTempUp = currentBtnTempUp;
    lastBtnMillis = millis();
  }
}

void renderDisplay() {
  Serial.println(fridgeTempSetpoint, DEC);
  serial2.write(fridgeTempSetpoint);

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(fridgeTempCurrent, DEC);
  display.drawCircle(38,2,2,WHITE);
  display.setTextSize(1);
  display.setCursor(64,0);
  display.print("Set: ");
  display.setTextSize(2);
  display.println(fridgeTempSetpoint);
  display.setTextSize(1);
  display.setCursor(64,12);
  display.print(pwmVal);
  display.setCursor(0,24);
  display.print("IP: "); display.print(WiFi.localIP());
  if (WiFi.status() == WL_CONNECTED) {
    display.drawBitmap(110, 19, wifi_connected, 15, 12, 1);
  } else {
    display.drawBitmap(110, 19, wifi_broken, 15, 12, 1);
  }
  display.display();
}

void loop() {
  server->handleClient();

  checkForTempUpdate();

  checkForButtonPress();
  
  if (millis() - lastMillis > 3000) {
    checkForAck();
    updateDisplay = true;
    if (updateDisplay) {
      renderDisplay();
      updateDisplay = false;
      lastMillis = millis();
    }
  }
}
