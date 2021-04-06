//by Pawel Merta
//doc : https://github.com/rotgamedev

// Used libraries 
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <PCF8574.h> //include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <BH1750.h>

//------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;
WiFiServer server(80);

// Variable to store the HTTP request
String header;

//Variables for wifimanager to store mqtt data
char friendly_name[40];
char mqtt_server[40];
char mqtt_port[10];
char mqtt_username[40];
char mqtt_password[40];

//Variables used in PubSub
String mqtt_server_pubsub;
String friendly_name_pubsub;
String mqtt_username_pubsub;
String mqtt_password_pubsub;

//DHT pins
#define DHTPIN 14 //pin in esp8266 for dht22 (D5)
#define DHTTYPE    DHT22

//hc-sr04 pins
#define trigPin 12 //D6
#define echoPin 13 //D7

//oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool displayOn=true; //Variable to determine if the OLED screen is turned on

//I2C addresses:
Adafruit_ADS1115 ads(0x48); //analog input expander
PCF8574 pcf8574(0x20);      //GPIO expander
BH1750 lightMeter (0x23);   //light intensity sensor GY-30

//DHT
DHT dht(DHTPIN, DHTTYPE);
float temp = 0.0; //variable to store temp.
float humidity = 0.0; //variable to store humidity

//bh1750 variable to store light intensity
uint16_t lux=0;

//Moisture sensor
const int airValue = 15530;   //Sensor value in the Air
const int waterValue = 6635;  //Sensor value in the Water

int soilMoisturePercent[4]={0,0,0,0}; //soil moisture percentages
int tempSoilMoisture[4]={0,0,0,0}; //temporary soil moisture percentages used to detect pump or sensor failure

//LED
int pinLed = 2;

//Plants variables for wifimanager
char f1_name[40]; //plant 1 name
char f1_min[5];   //plant 1 min moisture
char f1_max[5];   //plant 1 max moisture
char f1_active[2];  //plant 1 variable to define is sensor in use

char f2_name[40]; //as above
char f2_min[5];
char f2_max[5];
char f2_active[2];

char f3_name[40]; //as above
char f3_min[5];
char f3_max[5];
char f3_active[2]; 

char f4_name[40]; //as above
char f4_min[5];
char f4_max[5];
char f4_active[2];

//Sensor stabilization interval for wifimanager
char sensor_stab[5];

//Hc-sr04 values for the specific tank used - should be adjusted
long h_time=0;
float h_distance=0;
const float minWaterLevel=3;
const float maxWaterLevel=17;
const float canHeight=22;
bool noWater=false;
int waterLvl=0; //percentage value

//btn
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
const int shortPressTime = 500;

//btn2
int lastState2 = LOW; // the previous state from the input pin
int currentState2; // the current reading from the input pin
unsigned long pressedTime2  = 0;
unsigned long releasedTime2 = 0;
const int shortPressTime2 = 500; 

//variable to determine if a configuration save is needed
bool shouldSaveConfig = false;

//varibales used for sensor reading
unsigned long previousMillis = 0;
const long interval = 10000; //sensor reading interval

//variables holds information whether it was possible to read the configuration
bool isMqttConfig=false; //for mqtt
bool isFlowerConfig=false; //for plants

//varibales used for stabilization of soil moisture sensors
unsigned long currentTime[4] = {0,0,0,0};
unsigned long prevTime[4] = {0,0,0,0};
unsigned long stabilizationTime = 300000; //stabilization of soil moisture sensors time
bool needStabilization[4]={false, false, false, false}; //determines whether a stabilization time after watering is needed

bool flowerWatering[4]={false,false,false,false}; //variable that stores data about the necessity of the watering process
bool pumpError[4]={false,false,false,false}; //variable that stores data about pump or sensro failure


//Oled scrolling
int xOled1,minX1;
int xOled2,minX2;
unsigned long prevOledTime = 0;
const long oledRefreshTime = 25;

String wifiState="";
bool mqttState= false;
String mqttStateMsg="";

//web
unsigned long currentTimeWeb = 0;
unsigned long previousTimeWeb = 0;
const long timeoutTime=2000;

//methods---------------------------------------------------------------------------

//performed when there is a configuration change in the wifimanager
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//performed when there is a connection error
void configModeCallback (WiFiManager *myWiFiManager) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,2);
    display.println("Connect to SSID");
    display.setCursor(0,17);
    display.println("WateringStation");
    display.setCursor(0,32);
    display.println("192.168.4.1");
    display.setCursor(0,47);
    display.println("to configure");
    display.display();
    display.clearDisplay();
}

//reading mqtt configuration file
void ReadMqttConfig(){
  if (SPIFFS.begin()) {
    Serial.println("Mounted file system");
    if (SPIFFS.exists("/config.json")) 
    {
      //file exists, reading and loading
      Serial.println("Reading mqtt config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("Opened mqtt config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");
          strcpy(friendly_name, json["friendly_name"]);          
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_username, json["mqtt_username"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          
          String serverIP=mqtt_server;
          String deviceName=friendly_name;
          
          if (serverIP=="" || serverIP==NULL || deviceName=="" || deviceName==NULL)
          {
            isMqttConfig=false;
          }
          else
          {
            isMqttConfig=true;
          }
          
        }
        else
        {
          Serial.println("Failed to load mqtt config!");
            //clean FS, for testing
            //SPIFFS.format();
            //delay(500);
            //wifiManager.resetSettings();
            //delay(500);
            //ESP.restart()
            //delay(1000);
            OledConfigFileError("config.json");
        }
      }
    }
    else
    {
      Serial.println("File does not exist!");
      OledConfigFileError("config.json");
    }
  } else {
    Serial.println("Failed to mount FS!");
  }
}

void OledConfigFileError(String fileName)
{
   display.clearDisplay();
   display.setTextSize(1);
   display.setCursor(0,2);
   display.println("The file");
   display.setCursor(0,17);
   display.println(fileName);
   display.setCursor(0,32);
   display.println("cannot be");
   display.setCursor(0,47);
   display.println("loaded!");
   display.display();
   display.clearDisplay();
   delay(2000);
}

//saving mqtt data to the json file (when shouldSaveConfig = true))
void SaveMqttConfig(){
  if (shouldSaveConfig) {
    Serial.println("Saving mqtt config file");
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["friendly_name"] = friendly_name;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing!");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
    //shouldSaveConfig = false;
  }
}

//reading plants configuration file
void ReadFlowerConfig()
{
  if (SPIFFS.begin())
  {
    Serial.println("Mounted file system");
    if (SPIFFS.exists("/flower.json"))
    {
      //file exists, reading and loading
      Serial.println("Reading flower config file");
      File configFile = SPIFFS.open("/flower.json", "r");
      if (configFile)
      {
        Serial.println("Opened flower config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");

          strcpy(f1_name, json["f1_name"]);          
          strcpy(f1_min, json["f1_min"]);
          strcpy(f1_max, json["f1_max"]);
          strcpy(f1_active, json["f1_active"]);

          strcpy(f2_name, json["f2_name"]);          
          strcpy(f2_min, json["f2_min"]);
          strcpy(f2_max, json["f2_max"]);
          strcpy(f2_active, json["f2_active"]);

          strcpy(f3_name, json["f3_name"]);          
          strcpy(f3_min, json["f3_min"]);
          strcpy(f3_max, json["f3_max"]);
          strcpy(f3_active, json["f3_active"]);

          strcpy(f4_name, json["f4_name"]);          
          strcpy(f4_min, json["f4_min"]);
          strcpy(f4_max, json["f4_max"]);
          strcpy(f4_active, json["f4_active"]);

          strcpy(sensor_stab, json["sensor_stab"]);

          isFlowerConfig=true;

        } else {
          Serial.println("Failed to load flower config!");
            //clean FS, for testing
            //SPIFFS.format();
            //delay(500);
            //wifiManager.resetSettings();
            //delay(500);
            //ESP.restart()
            //delay(1000);
            OledConfigFileError("flower.json");
        }
      }
    }
    else
    {
      Serial.println("File does not exist!");
      OledConfigFileError("flower.json");
    }
  }
  else
  {
    Serial.println("Failed to mount FS!");
  }
}

//saving plants data to the json file (when shouldSaveConfig = true))
void SaveFlowerConfig(){
  if (shouldSaveConfig) {
    Serial.println("Saving plants config file");
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["f1_name"] = f1_name;
    json["f1_min"] = f1_min;
    json["f1_max"] = f1_max;
    json["f1_active"] = f1_active;
    json["f2_name"] = f2_name;
    json["f2_min"] = f2_min;
    json["f2_max"] = f2_max;
    json["f2_active"] = f2_active;
    json["f3_name"] = f3_name;
    json["f3_min"] = f3_min;
    json["f3_max"] = f3_max;
    json["f3_active"] = f3_active;
    json["f4_name"] = f4_name;
    json["f4_min"] = f4_min;
    json["f4_max"] = f4_max;
    json["f4_active"] = f4_active;
    json["sensor_stab"] = sensor_stab;

    File configFile = SPIFFS.open("/flower.json", "w");
    if (!configFile)
    {
      Serial.println("Failed to open config file for writing!");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
    //shouldSaveConfig = false;
  }
}

//wifimanager configuration metda
void WifiSet(){

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // set dark theme
  wifiManager.setClass("invert"); 
  //wifiManager.setConfigPortalTimeout(180);
  //wifiManager.setAPClientCheck(true); //if true, reset timeout when webclient connects (true), suggest disabling if captiveportal is open
  
  char customhtml_checkbox_f1[24]="type=\"checkbox\""; //checbox in web wifimanager
  char customhtml_checkbox_f2[24]="type=\"checkbox\"";
  char customhtml_checkbox_f3[24]="type=\"checkbox\"";
  char customhtml_checkbox_f4[24]="type=\"checkbox\"";
  
  if (strcmp(f1_active, "1")==0){
    strcat(customhtml_checkbox_f1, "checked"); //setting the state of the checkbox based on the configuration of plants
  }
  if (strcmp(f2_active, "1")==0){
    strcat(customhtml_checkbox_f2, "checked");
  }
  if (strcmp(f3_active, "1")==0){
    strcat(customhtml_checkbox_f3, "checked");
  }
  if (strcmp(f4_active, "1")==0){
    strcat(customhtml_checkbox_f4, "checked");
  }
   
  //labels
  WiFiManagerParameter custom_mqtt_label("<p>MQTT server credentials</p>");
  WiFiManagerParameter custom_flower_label("<p>Plants configuration</p>");
  WiFiManagerParameter custom_flower1_label("<p>Plant 1:</p>");
  WiFiManagerParameter custom_flower2_label("<p>Plant 2:</p>");
  WiFiManagerParameter custom_flower3_label("<p>Plant 3:</p>");
  WiFiManagerParameter custom_flower4_label("<p>Plant 4:</p>");
  WiFiManagerParameter custom_moisture_label("<p>Moisture (5-85%)</p>");
  WiFiManagerParameter custom_stab_label("<p>Sensor stabilization time [min] (1-60)</p>");
  
  //mqtt parameters
  WiFiManagerParameter custom_friendly_name("friendly_name", "Friendly Name", friendly_name, 40);
  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_username("mqtt_username", "MQTT Username", mqtt_username, 40);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqtt_password, 40);

  //plants parameters
  WiFiManagerParameter custom_f1_name("f1_name", "Name", f1_name, 35);
  WiFiManagerParameter custom_f1_min("f1_min", "Min", f1_min, 2);
  WiFiManagerParameter custom_f1_max("f1_max", "Max", f1_max, 2);
  WiFiManagerParameter custom_f1_active("f1_active","Is Active","T",2,customhtml_checkbox_f1,WFM_LABEL_BEFORE);
 
  WiFiManagerParameter custom_f2_name("f2_name", "Name", f2_name, 35);
  WiFiManagerParameter custom_f2_min("f2_min", "Min", f2_min, 2);
  WiFiManagerParameter custom_f2_max("f2_max", "Max", f2_max, 2);
  WiFiManagerParameter custom_f2_active("f2_active","Is Active","T",2,customhtml_checkbox_f2,WFM_LABEL_BEFORE);

  WiFiManagerParameter custom_f3_name("f3_name", "Name", f3_name, 35);
  WiFiManagerParameter custom_f3_min("f3_min", "Min", f3_min, 2);
  WiFiManagerParameter custom_f3_max("f3_max", "Max", f3_max, 2);
  WiFiManagerParameter custom_f3_active("f3_active","Is Active","T",2,customhtml_checkbox_f3,WFM_LABEL_BEFORE);

  WiFiManagerParameter custom_f4_name("f4_name", "Name", f4_name, 35);
  WiFiManagerParameter custom_f4_min("f4_min", "Min", f4_min, 2);
  WiFiManagerParameter custom_f4_max("f4_max", "Max", f4_max, 2);
  WiFiManagerParameter custom_f4_active("f4_active","Is Active","T",2,customhtml_checkbox_f4,WFM_LABEL_BEFORE);

  WiFiManagerParameter custom_sensor_stab("sensor_stab", "Stabilization time", sensor_stab, 2);

  wifiManager.addParameter(&custom_mqtt_label);
  wifiManager.addParameter(&custom_friendly_name);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  
  wifiManager.addParameter(&custom_flower_label);
  
  wifiManager.addParameter(&custom_flower1_label);
  wifiManager.addParameter(&custom_f1_name);
  wifiManager.addParameter(&custom_moisture_label);
  wifiManager.addParameter(&custom_f1_min);
  wifiManager.addParameter(&custom_f1_max);
  wifiManager.addParameter(&custom_f1_active);
  
  wifiManager.addParameter(&custom_flower2_label);
  wifiManager.addParameter(&custom_f2_name);
  wifiManager.addParameter(&custom_moisture_label);
  wifiManager.addParameter(&custom_f2_min);
  wifiManager.addParameter(&custom_f2_max);
  wifiManager.addParameter(&custom_f2_active);
  
  wifiManager.addParameter(&custom_flower3_label);
  wifiManager.addParameter(&custom_f3_name);
  wifiManager.addParameter(&custom_moisture_label);
  wifiManager.addParameter(&custom_f3_min);
  wifiManager.addParameter(&custom_f3_max);
  wifiManager.addParameter(&custom_f3_active);
  
  wifiManager.addParameter(&custom_flower4_label);
  wifiManager.addParameter(&custom_f4_name);
  wifiManager.addParameter(&custom_moisture_label);
  wifiManager.addParameter(&custom_f4_min);
  wifiManager.addParameter(&custom_f4_max);
  wifiManager.addParameter(&custom_f4_active);

  wifiManager.addParameter(&custom_stab_label);
  wifiManager.addParameter(&custom_sensor_stab);
    
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //when connection to wifi fail execute this callback
  wifiManager.setAPCallback(configModeCallback);

   // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  bool startWifiConnection;
  startWifiConnection = wifiManager.autoConnect("WateringStation","qwerty123"); // password protected ap  
  if(!startWifiConnection)
  {
    Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
    isMqttConfig=false;
  } 
  else
  {
    //if you get here you have connected to the WiFi    
    Serial.println("Connection succeeded!");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,10);
    display.println("Connected");
    display.setCursor(0,40);
    display.println("to Wifi");
    display.display();
    delay(5000);
    display.clearDisplay();
  }

  //assigning data to variables - mqtt and plants
  strcpy(friendly_name, custom_friendly_name.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_username, custom_mqtt_username.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());

  //plants
  strcpy(f1_name, custom_f1_name.getValue());
  strcpy(f1_min, custom_f1_min.getValue());
  strcpy(f1_max, custom_f1_max.getValue());

  if(isDigit(f1_min[0]) && (isDigit(f1_min[1]) || f1_min[1]==NULL) && (isDigit(f1_max[0]) && isDigit(f1_max[1])))
  {
    if (strncmp(custom_f1_active.getValue(),"T",1)==0 && atoi(f1_min)>4 && atoi(f1_max)<81 && atoi(f1_min)<atoi(f1_max))
    {
      strcpy(f1_active,"1");
    }
    else
    {
      strcpy(f1_active,"0");
    }
  }
  else
  {
    strcpy(f1_active,"0");
    strcpy(f1_min,"10");
    strcpy(f1_max,"60");
  }
  
  strcpy(f2_name, custom_f2_name.getValue());
  strcpy(f2_min, custom_f2_min.getValue());
  strcpy(f2_max, custom_f2_max.getValue());

  if(isDigit(f2_min[0]) && (isDigit(f2_min[1]) || f2_min[1]==NULL) && (isDigit(f2_max[0]) && isDigit(f2_max[1])))
  {
    if (strncmp(custom_f2_active.getValue(),"T",1)==0 && atoi(f2_min)>4 && atoi(f2_max)<81 && atoi(f2_min)<atoi(f2_max) )
    {
      strcpy(f2_active,"1");
    }
    else
    {
      strcpy(f2_active,"0");
    }
  }
  else
  {
    strcpy(f2_active,"0");
    strcpy(f2_min,"10");
    strcpy(f2_max,"60");
  }
  
  strcpy(f3_name, custom_f3_name.getValue());
  strcpy(f3_min, custom_f3_min.getValue());
  strcpy(f3_max, custom_f3_max.getValue());
  
  if(isDigit(f3_min[0]) && (isDigit(f3_min[1]) || f3_min[1]==NULL) && (isDigit(f3_max[0]) && isDigit(f3_max[1])))
  {
    if (strncmp(custom_f3_active.getValue(),"T",1)==0 && atoi(f3_min)>4 && atoi(f3_max)<81 && atoi(f3_min)<atoi(f3_max))
    {
      strcpy(f3_active,"1");
    }
    else
    {
      strcpy(f3_active,"0");
    }
  }
  else
  {
    strcpy(f3_active,"0");
    strcpy(f3_min,"10");
    strcpy(f3_max,"60");
  }
  
  strcpy(f4_name, custom_f4_name.getValue());
  strcpy(f4_min, custom_f4_min.getValue());
  strcpy(f4_max, custom_f4_max.getValue());

  if(isDigit(f4_min[0]) && (isDigit(f4_min[1]) || f4_min[1]==NULL) && (isDigit(f4_max[0]) && isDigit(f4_max[1])))
  {
    if (strncmp(custom_f4_active.getValue(),"T",1)==0 && atoi(f4_min)>4 && atoi(f4_max)<81 && atoi(f4_min)<atoi(f4_max))
    {
      strcpy(f4_active,"1");
    }
    else
    {
      strcpy(f4_active,"0");
    }
  }
  else
  {
    strcpy(f4_active,"0");
    strcpy(f4_min,"10");
    strcpy(f4_max,"60");
  }

  strcpy(sensor_stab, custom_sensor_stab.getValue());
  
  if(isDigit(sensor_stab[0]) && (isDigit(sensor_stab[1]) || sensor_stab[1]==NULL) && (isDigit(sensor_stab[0]) && isDigit(sensor_stab[1])))
  {
    if (atoi(sensor_stab)<1)
    {
      strcpy(sensor_stab,"5");
    }
    else if (atoi(sensor_stab)>60)
    {
      strcpy(sensor_stab,"60");
    }
  }
  else
  {
    strcpy(sensor_stab,"5");
  }
  
}

void setup() {
  Serial.begin(115200);

  pcf8574.begin();
  dht.begin();
  ads.begin();
  Wire.begin();
  lightMeter.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  xOled1=display.width();
  xOled2=display.width();

  pcf8574.pinMode(P0, OUTPUT); //delay 1
  pcf8574.pinMode(P1, OUTPUT); //delay 2
  pcf8574.pinMode(P2, OUTPUT); //delay 3
  pcf8574.pinMode(P3, OUTPUT); //delay 4
  pcf8574.pinMode(P4, OUTPUT); //no used
  pcf8574.pinMode(P5, INPUT); //button2
  pcf8574.pinMode(P6, OUTPUT); //RED LED
  pcf8574.pinMode(P7, INPUT); //button
  pcf8574.digitalWrite(P0, HIGH); //initial delay off
  pcf8574.digitalWrite(P1, HIGH);
  pcf8574.digitalWrite(P2, HIGH);
  pcf8574.digitalWrite(P3, HIGH);
  pcf8574.digitalWrite(P6, HIGH); //redled initial off
  
  // initialize GPIO 2 as an output.
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, HIGH); //off
 
  // initalize GPIOs fo HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, HIGH);//initial off
  
  //welcome oled message
  display.clearDisplay(); 
  display.setTextSize(2);
  display.setCursor(0,22);
  display.println("Starting..");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0,22);
  display.println("Reading");
  display.setCursor(0,40);
  display.println("sensors...");
  display.display();
  delay(3000);
 
  //first reead sensors
  TempHumRead();
  LightRead();
  MoistureSensorsRead();
  WaterLevelRead();
  
  //read mqtt config
  ReadMqttConfig();
  //read flower config
  ReadFlowerConfig();

  //start wifi connection etc
  WifiSet();

  //save mqtt config if need
  SaveMqttConfig();
  //save plants config if need
  SaveFlowerConfig();
  
  //disabling the configuration save variable
  shouldSaveConfig = false;

  if (isMqttConfig)
  {
    mqtt_server_pubsub=mqtt_server;
    friendly_name_pubsub=friendly_name;
    mqtt_username_pubsub=mqtt_username;
    mqtt_password_pubsub=mqtt_password;
  
    client.setBufferSize(255);
    client.setServer(mqtt_server_pubsub.c_str(),atoi(mqtt_port));
  }

  stabilizationTime=atoi(sensor_stab)*60000;
  Serial.println("Sensor stabilization time: "+String(stabilizationTime) + " ms");
  server.begin();
  Serial.println("End setup");
}

void BtnCheck()
{
    currentState = pcf8574.digitalRead(P7,true);
    
    if(lastState == LOW && currentState == HIGH)
    {        
      // button is pressed
      pressedTime = millis();
    }
    else if(lastState == HIGH && currentState == LOW)
    {
      // button is released
      releasedTime = millis();

      long pressDuration = releasedTime - pressedTime;

      if( pressDuration < shortPressTime )
      {
        Serial.println("A short press is detected");
        if (displayOn)
        {
          ShowSavedFlowerConfigOled();
        }
        else
        {
          display.ssd1306_command(SSD1306_DISPLAYON);
          ShowSavedFlowerConfigOled();
          display.ssd1306_command(SSD1306_DISPLAYOFF);
        } 
        
       /* 
        if (strcmp(mqtt_port, "1883")==0)
        {
          Serial.println("OK");
        } else {
          Serial.println("Not OK");
        }

        int val = atoi(mqtt_port);
        
        String name123=friendly_name;
       
        Serial.print("second char is: ");
        Serial.println(f2_min[1]);

        if(isDigit(f2_min[0]) && (isDigit(f2_min[1]) || f2_min[1]==NULL))
        {
          Serial.println("f1_min is a number");
        }
        else
        {
          Serial.println("f1_min is not a number");
        }*/
          
      }
      else if (pressDuration > shortPressTime && pressDuration < 5000)
      {
        Serial.println("A long press is detected");
        if (!displayOn)
        {
          display.ssd1306_command(SSD1306_DISPLAYON);
        }
        
        display.clearDisplay(); 
        display.setTextSize(2);
        display.setCursor(0,22);
        display.println("Rebooting..");
        display.display();
        delay(2000);
        ESP.restart();
      }
      else
      {
        Serial.println("A very long press is detected");
        if (!displayOn)
        {
          display.ssd1306_command(SSD1306_DISPLAYON);
        }
        /*if(SPIFFS.format())
        {
          Serial.println("File system Formatted");
        }
        else
        {
          Serial.println("File system formatting error");
        }*/
        display.clearDisplay(); 
        display.setTextSize(2);
        display.setCursor(0,22);
        display.println("Resetting..");
        display.display();
        delay(2000);
        
        WiFi.disconnect(true);
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart();
      }
    }

  // save the the last state
  lastState = currentState;
}

void Btn2Check()
{
  currentState2 = pcf8574.digitalRead(P5,true);
    
  if(lastState2 == LOW && currentState2 == HIGH)
  {        
    // button is pressed
    pressedTime2 = millis();
  }
  else if(lastState2 == HIGH && currentState2 == LOW)
  {
    // button is released
    releasedTime2 = millis();

    long pressDuration2 = releasedTime2 - pressedTime2;
    if( pressDuration2 < shortPressTime2 )
    {
      Serial.println("A short press is detected");
      if (displayOn)
      {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        Serial.println("Display OFF");
        displayOn=false;
      }
      else
      {
        display.ssd1306_command(SSD1306_DISPLAYON);
        Serial.println("Display ON");
        displayOn=true;
      }

    }
    else if (pressDuration2 > shortPressTime2 && pressDuration2 < 5000)
    {
      //not used
    }
    else
    {
      display.clearDisplay(); 
      display.setTextSize(2);
      display.setCursor(0,22);
      display.println("Formatting");
      display.setCursor(0,36);
      display.print("memory...");
      display.display();
      delay(2000);
      if(SPIFFS.format())
      {
        Serial.println("File system Formatted");
      }
      else
      {
        Serial.println("File system formatting error");
      }
      WiFi.disconnect(true);
      wifiManager.resetSettings();
      delay(1000);
      ESP.restart();
    }
  }
  lastState2 = currentState2;
}

//low water level led on
void WarningLedON()
{
  pcf8574.digitalWrite(P6, LOW);
}

//low water level led off
void WarningLedOFF()
{
  pcf8574.digitalWrite(P6, HIGH);
}

void publishAirData(float p_temperature, float p_humidity, uint16_t p_light) {
  String airTopic = friendly_name_pubsub+"/air";
  
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // INFO: the data must be converted into a string;
  root["temperature"] = (String)p_temperature;
  root["humidity"] = (String)p_humidity;
  root["light"] = (String)p_light;
  root["water"] = (String)waterLvl;
  root.prettyPrintTo(Serial);
  //Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70",
        "light": "12"
        "water": "80"
     }
  */
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(airTopic.c_str(), data, true);
  yield();
}

void publishFlowerData(int flower_id) {
  
  String msgConfigError = "not configured";
  
  String flowerTopic = friendly_name_pubsub+"/flower"+String(flower_id);

  String flowerName[4]={String(f1_name),String(f2_name),String(f3_name),String(f4_name)};
  String flowerMoistures[4]={String(soilMoisturePercent[0]),String(soilMoisturePercent[1]),String(soilMoisturePercent[2]),String(soilMoisturePercent[3])};

  int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
  for (int i = 0; i <= 3; i++)
   {
     if(activeValue[i]!=1)
     {
       flowerName[i]=msgConfigError;
       flowerMoistures[i]=msgConfigError;
     }
   }

  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // INFO: the data must be converted into a string;
  root["flower"+String(flower_id)+"_name"] = String(flowerName[flower_id-1]);
  root["flower"+String(flower_id)+"_moisture"] = String(flowerMoistures[flower_id-1]);
  
  root.prettyPrintTo(Serial);
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(flowerTopic.c_str(), data, true);
  yield();
}

// the loop function runs over and over again forever
void loop()
{
  unsigned long currentMillis = millis();
  BtnCheck();
  Btn2Check();

  int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
  
  if (currentMillis - prevOledTime >= oledRefreshTime)  //oled refresh interval
  {
    prevOledTime = currentMillis;
    OledPrint();
  } 
 
  if (currentMillis - previousMillis >= interval)  //sensors read interval
  {
    previousMillis = currentMillis;
    TempHumRead(); //read air temperature and humidity
    LightRead();//read light intensity
    MoistureSensorsRead(); //soil moisture reading
    WaterLevelRead(); //check water lvl in tank
    WateringFlowers(); //start watering process if needed
    CheckWifiState(); //check wifi state
    
    if (isMqttConfig) //when the configuration is created it sends messages to the mqtt server
    {
      if (client.connect(friendly_name_pubsub.c_str(),mqtt_username_pubsub.c_str(),mqtt_password_pubsub.c_str()))
      {
        mqttState=true;
        publishAirData(temp,humidity,lux);
        for (int i = 0; i <= 3; i++)
        {
          if(activeValue[i]==1)
          {
            publishFlowerData(i+1);
          }
        }
        Serial.println("Message send");
      }
      else
      {
        mqttState=false;
      }
    }
    CheckMQTTState();
  }

  WebPage();

  for (int i = 0; i <= 3; i++) //countdown of the stabilization time of sensors
  {
    if(needStabilization[i])
    {
      currentTime[i] = millis();
    
      if (currentTime[i] - prevTime[i] >= stabilizationTime)
      {
        needStabilization[i]=false;
      }
    }
  }
  
}

//read light intensity
void LightRead()
{
    lux = lightMeter.readLightLevel();
    Serial.println("Light: " + String(lux) + " lx"); 
}

//read air temperture and humidity
void TempHumRead()
{
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      temp = newT;
      Serial.println("Temperature " + String(temp) + " C"); 
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      humidity = newH;
      Serial.println("Humidity " + String(humidity) + " %");
    }
}


//read moisture sensors
void MoistureSensorsRead()
{
  int16_t adc[4];
  
  for (int i = 0; i <= 3; i++)
  {
    adc[i]= ads.readADC_SingleEnded(i);
    soilMoisturePercent[i] = map(adc[i], airValue, waterValue, 0, 100);
  }
  
  for (int i = 0; i <= 3; i++)
  {
    if(soilMoisturePercent[i] > 100)
    {
      Serial.println("Sensor "+String(i+1)+": Humidity: 100 %");
      soilMoisturePercent[i]=100;
    }
    else if(soilMoisturePercent[i] <0)
    {
      Serial.println("Sensor "+String(i+1)+": Humidity: 0 %");
      soilMoisturePercent[i]=0;
    }
    else if(soilMoisturePercent[i] >=0 && soilMoisturePercent[i] <= 100)
    {
      Serial.println("Sensor "+String(i+1)+": Humidity: " + String(soilMoisturePercent[i]) + " %");
    }
  }
  
}

//watering
void WateringFlowers()
{
  int flowerMoistureAvrg[4]={0,0,0,0};

  if (isFlowerConfig && !noWater)
  {
    int isActive[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
    int flowerMin[4]={atoi(f1_min), atoi(f2_min), atoi(f3_min), atoi(f4_min)};
    int flowerMax[4]={atoi(f1_max), atoi(f2_max), atoi(f3_max), atoi(f4_max)};
  
    for (int i = 0; i <= 3; i++)
    {
      if (isActive[i]==1 && !needStabilization[i])
      {
        flowerMoistureAvrg[i]=(flowerMin[i]+flowerMax[i])/2;

        if(flowerMoistureAvrg[i]!=0)
        {
            if (flowerWatering[i])
            {
              if(soilMoisturePercent[i]<flowerMoistureAvrg[i])
              {
                if(soilMoisturePercent[i]<=tempSoilMoisture[i])
                {
                  pumpError[i]=true;
                }
                else
                {
                  if (pumpError[i])
                  {
                    pumpError[i]=false;
                  }
                  RunPump(i,2000);
                  tempSoilMoisture[i]=soilMoisturePercent[i];
                }
              }
              else
              {
                flowerWatering[i]=false;
                Serial.println("Watering process completed for flower no: " + String(i+1));
                tempSoilMoisture[i]=0;
              }
            }        
            else if (soilMoisturePercent[i]<flowerMin[i] && !needStabilization[i])
            {
              flowerWatering[i]=true;
              Serial.println("Watering process started for flower no: " + String(i+1));
              if (!pumpError[i])
              {
                RunPump(i,2000); //nr pompy i czas podlewania
                tempSoilMoisture[i]=soilMoisturePercent[i]; //save temporary moisture
              }
            }  
        }        
      }
    }
  }
  else if (noWater)
  {
    for (int i = 0; i <= 3; i++)
    {
      flowerWatering[i]=false;
    }
    Serial.println("Watering plants completed due to lack of water.");
  }
}


//starting the selected water pump
void RunPump(int pumpNr, long duratioin)
{
  WaterLevelRead();
  if (!noWater && !needStabilization[pumpNr])
  {
    pcf8574.digitalWrite(pumpNr, LOW); //run pump
    Serial.println("Pump no: " + String(pumpNr)+" is running");
    OledWatering(pumpNr);
    delay(duratioin);
    pcf8574.digitalWrite(pumpNr, HIGH); //stop pump
    delay(250);
    Serial.println("Pump no: " + String(pumpNr)+" has stoped");
    needStabilization[pumpNr]=true;
    prevTime[pumpNr]=millis();
  }
}

void CheckWifiState()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiState="OK";
  }
  else
  {
    wifiState="NO!";
  }
}

void CheckMQTTState()
{
  if (mqttState)
  {
    mqttStateMsg="OK";
  }
  else
  {
    mqttStateMsg="NO!";
  }
}

void OledPrint()
{
    String msgConfigError = "-"; //"not configured";
    String msgSensorStabilization = "Sensor stabilization";
    String messageFlower1 = "1. Moisture -  " + String(f1_name) + "  " + String(soilMoisturePercent[0]) + " %";
    String messageFlower2 = "2. Moisture -  " + String(f2_name) + "  " + String(soilMoisturePercent[1]) + " %";
    String messageFlower3 = "3. Moisture -  " + String(f3_name) + "  " + String(soilMoisturePercent[2]) + " %";
    String messageFlower4 = "4. Moisture -  " + String(f4_name) + "  " + String(soilMoisturePercent[3]) + " %";
    
    int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
    String msgFlower[4]={messageFlower1,messageFlower2,messageFlower3,messageFlower4};
    
    for (int i = 0; i <= 3; i++)
    {
      if(activeValue[i]!=1)
      {
        msgFlower[i]=msgConfigError;
      }
      else
      {
        if (pumpError[i])
        {
          msgFlower[i]="Pump " +String(i+1)+ " or sensor "+String(i+1)+" failure!";
        }
        else if (needStabilization[i])
        {
          msgFlower[i]+=" "+msgSensorStabilization;
        }
      } 
    }

    char messageF1[msgFlower[0].length()+1];
    strcpy(messageF1, msgFlower[0].c_str());

    char messageF2[msgFlower[1].length()+1];
    strcpy(messageF2, msgFlower[1].c_str());

    char messageF3[msgFlower[2].length()+1];
    strcpy(messageF3, msgFlower[2].c_str());
    
    char messageF4[msgFlower[3].length()+1];
    strcpy(messageF4, msgFlower[3].c_str());
    
    int m1=strlen(messageF1);
    int m2=strlen(messageF2);
    int m3=strlen(messageF3);
    int m4=strlen(messageF4);

    int msgMax1 = max(m1,m2);
    int msMax2 = max(m3,m4);
    int finalMax = max(msgMax1,msMax2);
    String tempUnit = "C"; //change do F if use Fahrenheit
    String messageAirEnv = "AIR: Temperature: " + String(temp) + " " + tempUnit + "  -  " + "Humidity: " + String(humidity) + " %  -  " + "Light: " + String(lux) + " lx";
    char message[messageAirEnv.length()+1];
    strcpy(message, messageAirEnv.c_str());
    
    minX1=-6*strlen(message);
    minX2=-6*finalMax;

    display.clearDisplay();

    display.setTextSize(1);
    
    display.setCursor(0,0);  //oled display
    display.println("WiFi: "+wifiState +"    MQTT: "+ mqttStateMsg);

    display.setCursor(xOled1,9);
    display.print(message);

    display.setCursor(xOled2,18);
    display.print(messageF1);
   
    display.setCursor(xOled2,27);
    display.print(messageF2);
    
    display.setCursor(xOled2,36);
    display.print(messageF3);
    
    display.setCursor(xOled2,45);
    display.print(messageF4);   
    
    display.setCursor(0,54);
    display.println("WATER LEVEL: "+String(waterLvl)+" %");
    
    display.display();
    xOled1=xOled1-1;//scroll speed
    xOled2=xOled2-1;

    if(xOled1<minX1) xOled1 = display.width();
    if(xOled2<minX2) xOled2 = display.width();

}

void OledWatering(int flower)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,10);
  display.println("Watering");
  display.setCursor(0,40);
  display.println("plant " +String(flower+1));
  display.display();
}

//read water lvl in tank
void WaterLevelRead()
{
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);

  h_time = pulseIn(echoPin, HIGH);
  h_distance=h_time/58;
  delay(50);
  //Serial.println(h_time);
  Serial.print(canHeight-h_distance);
  Serial.println (" cm");

  waterLvl = map((canHeight-h_distance), minWaterLevel, maxWaterLevel, 0, 100);
  if(waterLvl > 100)
  {
    Serial.println("Water Level: 100 %");
    waterLvl=100;
  }
  else if(waterLvl <0)
  {
    Serial.println("Water Level: 0 %");
    waterLvl=0;
  }
  else if(waterLvl >=0 && waterLvl <= 100)
  {
    Serial.println("Water Level: " + String(waterLvl) + " %");
  }
  
  if (canHeight-h_distance <= minWaterLevel)
  {
    noWater=true;
    Serial.println ("No Water!");
    WarningLedON(); 
  }
  else
  {
    noWater=false;
    WarningLedOFF(); 
  }
}

void ShowSavedFlowerConfigOled()
{
    String messageFlower1 = "1. " + String(f1_name) + "  " + String(f1_min) + " / " + String(f1_max);
    String messageFlower2 = "2. " + String(f2_name) + "  " + String(f2_min) + " / " + String(f2_max);
    String messageFlower3 = "3. " + String(f3_name) + "  " + String(f3_min) + " / " + String(f3_max);
    String messageFlower4 = "4. " + String(f4_name) + "  " + String(f4_min) + " / " + String(f4_max);

    String msgConfigError = "not configured";
    
    int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
    String msgFlower[4]={messageFlower1,messageFlower2,messageFlower3,messageFlower4};
    
    for (int i = 0; i <= 3; i++)
    {
      if(activeValue[i]!=1)
      {
        msgFlower[i]=msgConfigError;
      }
      
    }
    
    display.clearDisplay();

    display.setTextSize(1);
    
    display.setCursor(0,0);  //oled display
    display.println("Saved Plants Config");

    display.setCursor(0,9);
    display.print(msgFlower[0]);

    display.setCursor(0,18);
    display.print(msgFlower[1]);
   
    display.setCursor(0,27);
    display.print(msgFlower[2]);
    
    display.setCursor(0,36);
    display.print(msgFlower[3]);
    
    display.setCursor(0,45);
    display.println("WiFi SSID:");   
    
    display.setCursor(0,54);
    display.println(WiFi.SSID());
    
    display.display();
    delay(5000);


}

void WebPage()
{
  WiFiClient client = server.available();   // Listen for incoming clients

  if (!client)// If a new client connects,
  {
    return;
  }
  else
  {                             
    currentTimeWeb=millis();
    previousTimeWeb=currentTimeWeb;
    
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTimeWeb - previousTimeWeb <= timeoutTime ) {            // loop while the client's connected
      currentTimeWeb=millis();
      
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            /*// turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("GPIO 5 on");
              //output5State = "on";
              //digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("GPIO 5 off");
              //output5State = "off";
              //digitalWrite(output5, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              //output4State = "on";
             // digitalWrite(output4, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              //output4State = "off";
             // digitalWrite(output4, LOW);
            }*/
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">");                          
            client.println("<style>");
            client.println("*{box-sizing: border-box;}");
            client.println("html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("h1 { font-size: 3.0rem;}");    
            client.println("h2 { font-size: 2.0rem;}");
            client.println("p { font-size: 2.0rem;}");
            client.println(".units { font-size: 1.2rem;}");
            client.println(".labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}");
            client.println(".column{ float:left; width:50%; padding: 10px;}");
            client.println(".row:after{ display: table; clear: both;}");
            client.println("</style>");
            //client.println("<script>function refresh(refreshPeriod){setTimeout(\"location.reload(true);\", refreshPeriod);} window.onload = refresh(30000)</script>");
            client.println("</head>");
            // Web Page Heading
            client.println("<body><h1>Watering Station</h1>");
            client.println("<h2>Air:</h2>");      

            client.println("<div class=\"row\"><div class=\"column\">");
            client.println("<p><i class=\"fas fa-sun\" style=\"color:#f2f20d;\"></i> <span class=\"labels\">Light:</span> <span>"+(String)lux+"</span> <sup class=\"units\">lx</sup></p>");
            client.println("<p><i class=\"fas fa-water\" style=\"color:#00add6;\"></i> <span class=\"labels\">Water Level:</span> <span>"+(String)waterLvl+"</span> <sup class=\"units\">&percnt;</sup></p>");
            client.println("</div><div class=\"column\">");
            client.println("<p><i class=\"fas fa-thermometer-half\" style=\"color:#059e8a;\"></i> <span class=\"labels\">Temperature:</span> <span>"+(String)temp+"</span> <sup class=\"units\">&deg;C</sup></p>");
            client.println("<p><i class=\"fas fa-tint\" style=\"color:#00add6;\"></i> <span class=\"labels\">Humidity:</span> <span>"+(String)humidity+"</span> <sup class=\"units\">&percnt;</sup></p>");
            client.println("</div></div>");
            
            client.println("<h2>Plants:</h2>");
       
            int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
            String flowerNames[4]={String(f1_name),String(f2_name),String(f3_name),String(f4_name)};
    
            for (int i = 0; i <= 3; i++)
            {
              if(activeValue[i]==1)
              {
                client.println("<p><i class=\"fas fa-seedling\" style=\"color:#00cc00;\"></i> <span>"+String(flowerNames[i])+"</span> <span class=\"labels\">Moisture:</span> <span>"+(String)soilMoisturePercent[i]+"</span> <sup class=\"units\">&percnt;</sup></p>");
              }
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {  // if you got anything else but a carriage return character,
          currentLine += c;// add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
