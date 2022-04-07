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
#include <NTPClient.h>
#include <WiFiUdp.h>

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
const int airValue = 7000;//7350;  //6730; //10625; //15530;   //Sensor value in dry soil
const int waterValue = 1430;//1400;//1430; //2400;  //6635;  //Sensor value in water

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
float lastDistance=0;
float currentDistance=0;
const float minWaterLevel=3;
const float maxWaterLevel=17;
const float canHeight=22;
bool noWater=false;
int waterLvl=0; //percentage value
bool firstRun=true;

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
const long interval = 300000;//1800000; //5min sensor reading interval

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
int errorCount[4]={0,0,0,0};
float wateringTime=2000;


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

//wifi state
int wifiStateCount=0;

// Define NTP Client to get time
const long utcOffsetInSeconds = 7200;

String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);
NTPClient timeClient(ntpUDP, "pl.pool.ntp.org", utcOffsetInSeconds);

String lastReadingTime="No reading since last run";
String lastWatering[4]={"No watering since last run","No watering since last run","No watering since last run","No watering since last run"};


//methods---------------------------------------------------------------------------

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
  OledStarting();
  delay(2000);
  OledReadingSensors();
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

  ReadMqttConfig();

  if (isMqttConfig)
  {
    mqtt_server_pubsub=mqtt_server;
    friendly_name_pubsub=friendly_name;
    mqtt_username_pubsub=mqtt_username;
    mqtt_password_pubsub=mqtt_password;
  
    client.setBufferSize(255);
    client.setServer(mqtt_server_pubsub.c_str(),atoi(mqtt_port));
    Serial.println("mqtt config");
  }

  stabilizationTime=atoi(sensor_stab)*60000;
  Serial.println("Sensor stabilization time: "+String(stabilizationTime) + " ms");
  CheckWifiState();
  if (WiFi.status() == WL_CONNECTED)
  {
    if (isMqttConfig) //when the configuration is created it sends messages to the mqtt server
    {
      if (client.connect(friendly_name_pubsub.c_str(),mqtt_username_pubsub.c_str(),mqtt_password_pubsub.c_str()))
      {
        int activeValue[4]={atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
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
    timeClient.begin();
  }
  CheckMQTTState();
  lastReadingTime=GetDateTime();
  server.begin();
  Serial.println("End setup");
}

// the loop function runs over and over again forever-----------------------------------------------------------------------
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
    OledReadingSensors();

    TempHumRead(); //read air temperature and humidity
    LightRead();//read light intensity
    MoistureSensorsRead(); //soil moisture reading
    WaterLevelRead(); //check water lvl in tank
    WateringFlowers(); //start watering process if needed
    CheckWifiState(); //check wifi state
    
    if (WiFi.status() == WL_CONNECTED)
    {
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
    }
    CheckMQTTState();
    lastReadingTime=GetDateTime();
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

String GetDateTime()
{
  String currentDateTime="";
  String msgConnectionError="No connection to Internet";
  if (WiFi.status() == WL_CONNECTED)
  {
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime);

    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    String currentMonthName = months[currentMonth-1];
    int currentYear = ptm->tm_year+1900;
    String currentDate = String(currentYear) + "." + String(currentMonth) + "." + String(monthDay);
    currentDateTime = currentMonthName + ", "+ currentDate + ", " + String(timeClient.getFormattedTime());
    Serial.println(currentDateTime);
    //currentDateTime=String(daysOfTheWeek[timeClient.getDay()])+", "+String(timeClient.getHours())+":"+String(timeClient.getMinutes())+":"+String(timeClient.getSeconds());
    //currentDateTime=String(weekDays[timeClient.getDay()])+", "+String(timeClient.getFormattedTime());
    Serial.println(timeClient.getFormattedTime());
    return currentDateTime;
  }
  else
  {
    return msgConnectionError;
  }
  
}

void CheckWifiState()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiState="OK";
    wifiStateCount=0;
  }
  else
  {
    wifiState="NO!";
    /*wifiStateCount++;
    //WiFi.begin();
    if (wifiStateCount==10)
    {
      OledRebooting();
      ESP.restart();
    }
    WiFi.reconnect();*/
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
        
        OledRebooting();
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
        OledResetting();
        
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
      OledFormatting();
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



//performed when there is a configuration change in the wifimanager
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//wifimanager configuration metda
void WifiSet(){

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // set dark theme
  wifiManager.setClass("invert"); 
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPClientCheck(true); //if true, reset timeout when webclient connects (true), suggest disabling if captiveportal is open
  
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
  WiFiManagerParameter custom_moisture_label("<p>Moisture (5-95%)</p>");
  WiFiManagerParameter custom_stab_label("<p>Sensor stabilization time [min] (30-300)</p>");
  
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

  WiFiManagerParameter custom_sensor_stab("sensor_stab", "Stabilization time", sensor_stab, 3);

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
    OledConnectedToWiFi();
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
    if (strncmp(custom_f1_active.getValue(),"T",1)==0 && atoi(f1_min)>4 && atoi(f1_max)<96 && atoi(f1_min)<atoi(f1_max))
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
    strcpy(f1_min,"25");
    strcpy(f1_max,"60");
  }
  
  strcpy(f2_name, custom_f2_name.getValue());
  strcpy(f2_min, custom_f2_min.getValue());
  strcpy(f2_max, custom_f2_max.getValue());

  if(isDigit(f2_min[0]) && (isDigit(f2_min[1]) || f2_min[1]==NULL) && (isDigit(f2_max[0]) && isDigit(f2_max[1])))
  {
    if (strncmp(custom_f2_active.getValue(),"T",1)==0 && atoi(f2_min)>4 && atoi(f2_max)<96 && atoi(f2_min)<atoi(f2_max) )
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
    strcpy(f2_min,"25");
    strcpy(f2_max,"60");
  }
  
  strcpy(f3_name, custom_f3_name.getValue());
  strcpy(f3_min, custom_f3_min.getValue());
  strcpy(f3_max, custom_f3_max.getValue());
  
  if(isDigit(f3_min[0]) && (isDigit(f3_min[1]) || f3_min[1]==NULL) && (isDigit(f3_max[0]) && isDigit(f3_max[1])))
  {
    if (strncmp(custom_f3_active.getValue(),"T",1)==0 && atoi(f3_min)>4 && atoi(f3_max)<96 && atoi(f3_min)<atoi(f3_max))
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
    strcpy(f3_min,"25");
    strcpy(f3_max,"60");
  }
  
  strcpy(f4_name, custom_f4_name.getValue());
  strcpy(f4_min, custom_f4_min.getValue());
  strcpy(f4_max, custom_f4_max.getValue());

  if(isDigit(f4_min[0]) && (isDigit(f4_min[1]) || f4_min[1]==NULL) && (isDigit(f4_max[0]) && isDigit(f4_max[1])))
  {
    if (strncmp(custom_f4_active.getValue(),"T",1)==0 && atoi(f4_min)>4 && atoi(f4_max)<96 && atoi(f4_min)<atoi(f4_max))
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
    strcpy(f4_min,"25");
    strcpy(f4_max,"60");
  }

  strcpy(sensor_stab, custom_sensor_stab.getValue());
    
  if(isDigit(sensor_stab[0]) && isDigit(sensor_stab[1]) && (isDigit(sensor_stab[2]) || sensor_stab[2]==NULL))
  {
    if (atoi(sensor_stab)<30)
    {
      strcpy(sensor_stab,"30");
    }
    else if (atoi(sensor_stab)>300)
    {
      strcpy(sensor_stab,"300");
    }
  }
  else
  {
    strcpy(sensor_stab,"30");
    Serial.println("Setting default sensor stabilization time");
  }
}
