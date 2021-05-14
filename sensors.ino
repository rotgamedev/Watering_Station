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
 // int16_t adc[4];
  int16_t adc;
  int32_t aM;
  int mn = 10;
  
  for (int i = 0; i <= 3; i++)
  {
    int16_t minValue=0;
    int16_t maxValue=0;
    
    int32_t sum = 0;
      
    for (int m = 0; m < mn; m++)
    {
       adc= ads.readADC_SingleEnded(i);
       
       if(m==0)
       {
        minValue=adc;
        maxValue=adc;
       }
       
       if (adc>maxValue)
       {
          maxValue=adc;
       }else if (adc<minValue)
       {
          minValue=adc;
       }
       sum+=adc;
       Serial.println(String(adc)+" "+String(i));

    }
    //Serial.println(sum);
    Serial.println("max value: "+ String(maxValue));
    Serial.println("min value: "+ String(minValue));
    Serial.println("suma - "+String(sum));
    aM=((sum-maxValue)-minValue)/(mn-2);
    
    //adc[i]= ads.readADC_SingleEnded(i);
    //soilMoisturePercent[i] = map(adc[i], airValue, waterValue, 0, 100);
    //Serial.println(adc[i]);
    soilMoisturePercent[i] = map(aM, airValue, waterValue, 0, 100);
    Serial.println("average value - "+ String(aM));
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

//read water lvl in tank
void WaterLevelRead()
{
  WaterLevelCheck();
  
  if (!firstRun)
  {
    if (AbnormalCheck(lastDistance, currentDistance))
    {
      Serial.print("Abnormal water level read"); 
    }
    else
    {
      WaterLevelSet();
    }
  }
  else
  {
    if (currentDistance < minWaterLevel)
    {
      WaterLevelCheck();      
      WaterLevelSet();
    }
    else
    {
      WaterLevelSet();
    }
  }
 
  firstRun=false;
}

// Check for significant jumps and outliers in sensor reading
bool AbnormalCheck(float lastRead, float thisRead) {

  // if the difference between measures is greater then 4cm, outlier found
  if ((lastRead - thisRead) > 4)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void WaterLevelCheck()
{
  lastDistance=currentDistance;
  
  int mn = 8;
  float minValue=0;
  float maxValue=0;
  float sum = 0;
  float measure=0;
  float averageValue=0;
  
  for (int m = 0; m < mn; m++)
  {
    measure = MeasureWaterLevel();
    if(m==0)
    {
      minValue=measure;
      maxValue=measure;
    }
    
    if (measure>maxValue)
    {
       maxValue=measure;
    }else if (measure<minValue)
    {
      minValue=measure;
    }
    
    sum+=measure;
  }

  averageValue=((sum-maxValue)-minValue)/(mn-2);
  currentDistance=averageValue;
}

float MeasureWaterLevel()
{
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);

  h_time = pulseIn(echoPin, HIGH);
  h_distance=h_time*0.034/2;  //h_time/58;
  delay(100);
  //Serial.println(h_time);
  currentDistance=canHeight-h_distance;
  Serial.print(currentDistance);
  Serial.println (" cm");
  return currentDistance;
}

void WaterLevelSet()
{
  waterLvl = map(currentDistance, minWaterLevel, maxWaterLevel, 0, 100);
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
  
  if (currentDistance < minWaterLevel)
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
