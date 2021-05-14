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
                  errorCount[i]++;
                  if (errorCount[i]==5)
                  {
                    pumpError[i]=true;
                  }
                  else
                  {
                    RunPump(i,wateringTime);
                    tempSoilMoisture[i]=soilMoisturePercent[i];               
                  }
                }
                else
                {
                  if (pumpError[i])
                  {
                    pumpError[i]=false;
                  }
                  errorCount[i]=0;
                  RunPump(i,wateringTime);
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
                RunPump(i,wateringTime); //nr pompy i czas podlewania
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
    lastWatering[pumpNr]=GetDateTime();

  }
}
