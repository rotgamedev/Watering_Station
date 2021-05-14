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

void ShowSavedFlowerConfigOled()
{
    String messageFlower1 = "1. " + String(f1_min) + "  min/max " + String(f1_max);
    String messageFlower2 = "2. " + String(f2_min) + " min/max " + String(f2_max);
    String messageFlower3 = "3. " + String(f3_min) + " min/max " + String(f3_max);
    String messageFlower4 = "4. " + String(f4_min) + " min/max " + String(f4_max);

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
    display.println(WiFi.localIP());

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

void OledReadingSensors()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,22);
  display.println("Reading");
  display.setCursor(0,40);
  display.println("sensors...");
  display.display();
}

void OledStarting()
{
  display.clearDisplay(); 
  display.setTextSize(2);
  display.setCursor(0,22);
  display.println("Starting..");
  display.display();
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

void OledConnectedToWiFi()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,10);
    display.println("Connected");
    display.setCursor(0,40);
    display.println("to WiFi");
    display.display();
    delay(3000);
}

void OledRebooting()
{
    display.clearDisplay(); 
    display.setTextSize(2);
    display.setCursor(0,22);
    display.println("Rebooting..");
    display.display();
    delay(2000);
}

void OledResetting()
{
    display.clearDisplay(); 
    display.setTextSize(2);
    display.setCursor(0,22);
    display.println("Resetting..");
    display.display();
    delay(2000);
}

void OledFormatting()
{
    display.clearDisplay(); 
    display.setTextSize(2);
    display.setCursor(0,22);
    display.println("Formatting");
    display.setCursor(0,36);
    display.print("memory...");
    display.display();
    delay(2000);
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
