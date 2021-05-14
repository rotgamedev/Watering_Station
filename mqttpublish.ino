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
  String flowerLastWatering[4]={lastWatering[0],lastWatering[1],lastWatering[2],lastWatering[3]};

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
  root["flower"+String(flower_id)+"_lastWatering"] = String(flowerLastWatering[flower_id-1]);
  
  root.prettyPrintTo(Serial);
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(flowerTopic.c_str(), data, true);
  yield();
}
