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
            Serial.println("No mqtt config");
          }
          else
          {
            isMqttConfig=true;
            Serial.println("Found mqtt config");
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
  }
}
