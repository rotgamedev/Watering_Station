void WebPage()
{
  WiFiClient client = server.available();   // Listen for incoming clients

  if (!client)// If a new client connects,
  {
    return;
  }
  else
  {
    currentTimeWeb = millis();
    previousTimeWeb = currentTimeWeb;

    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTimeWeb - previousTimeWeb <= timeoutTime ) {            // loop while the client's connected
      currentTimeWeb = millis();

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

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">");
            client.println("<style>");
            client.println("*{box-sizing: border-box;}");
            client.println("html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; color: white;}");
            client.println("h1 { font-size: 3.0rem;}");
            client.println("h2 { font-size: 2.0rem;}");
            client.println("p { font-size: 2.0rem;}");
            client.println(".units { font-size: 1.2rem;}");
            client.println(".labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}");
            client.println(".labels2{ font-size: 1rem; vertical-align:middle; padding-bottom: 15px;}");
            client.println(".column{ float:left; width:50%; padding: 10px;}");
            client.println(".row:after{ display: table; clear: both;}");
            client.println("</style>");
            //client.println("<script>function refresh(refreshPeriod){setTimeout(\"location.reload(true);\", refreshPeriod);} window.onload = refresh(30000)</script>");
            client.println("</head>");
            // Web Page Heading
            client.println("<body style=\"background-color:#242424;\"><h1>Watering Station</h1>");
            client.println("<h2>Air:</h2>");

            client.println("<div class=\"row\"><div class=\"column\">");
            client.println("<p><i class=\"fas fa-sun\" style=\"color:#f2f20d;\"></i> <span class=\"labels\">Light:</span> <span>" + (String)lux + "</span> <sup class=\"units\">lx</sup></p>");
            client.println("<p><i class=\"fas fa-water\" style=\"color:#00add6;\"></i> <span class=\"labels\">Water Level:</span> <span>" + (String)waterLvl + "</span> <sup class=\"units\">&percnt;</sup></p>");
            client.println("</div><div class=\"column\">");
            client.println("<p><i class=\"fas fa-thermometer-half\" style=\"color:#059e8a;\"></i> <span class=\"labels\">Temperature:</span> <span>" + (String)temp + "</span> <sup class=\"units\">&deg;C</sup></p>");
            client.println("<p><i class=\"fas fa-tint\" style=\"color:#00add6;\"></i> <span class=\"labels\">Humidity:</span> <span>" + (String)humidity + "</span> <sup class=\"units\">&percnt;</sup></p>");
            client.println("</div></div>");

            client.println("<h2>Plants:</h2>");

            int activeValue[4] = {atoi(f1_active), atoi(f2_active), atoi(f3_active), atoi(f4_active)};
            String flowerNames[4] = {String(f1_name), String(f2_name), String(f3_name), String(f4_name)};

            for (int i = 0; i <= 3; i++)
            {
              if (activeValue[i] == 1)
              {
                if (pumpError[i])
                {
                  client.println("<p><i class=\"fas fa-seedling\" style=\"color:#00cc00;\"></i> <span>" + String(flowerNames[i]) + "</span> <span class=\"labels\">Moisture:</span> <span>" + String(soilMoisturePercent[i]) + "</span> <sup class=\"units\">&percnt;</sup> <span class=\"labels\">(pump or sensor error)</span></p>");
                }
                else
                {
                  if (needStabilization[i] == true)
                  {
                    client.println("<p><i class=\"fas fa-seedling\" style=\"color:#00cc00;\"></i> <span>" + String(flowerNames[i]) + "</span> <span class=\"labels\">Moisture:</span> <span>" + String(soilMoisturePercent[i]) + "</span> <sup class=\"units\">&percnt;</sup> <span class=\"labels\">(stabilization)</span></p>");
                  }
                  else
                  {
                    client.println("<p><i class=\"fas fa-seedling\" style=\"color:#00cc00;\"></i> <span>" + String(flowerNames[i]) + "</span> <span class=\"labels\">Moisture:</span> <span>" + String(soilMoisturePercent[i]) + "</span> <sup class=\"units\">&percnt;</sup></p>");
                  }
                }

                if (flowerWatering[i])
                {
                  client.println("<p><span class=\"labels2\">Watering is in progress</span></p>");
                }
                client.println("<p><span class=\"labels2\">Last watering:</span> <span class=\"labels2\">" + lastWatering[i] + "</span></p>");
              }
            }

            client.println("<div class=\"row\"><div class=\"column\">");
            client.println("<p><i class=\"fas fa-stopwatch\" style=\"color:#476b6b;\"></i> <span class=\"labels\">Sensors read interval:</span> <span>" + (String(interval / 60000)) + "</span> <sup class=\"units\">min.</sup></p>");
            client.println("<p><i class=\"fas fa-stopwatch\" style=\"color:#476b6b;\"></i> <span class=\"labels\">Stabilization time:</span> <span>" + (String(stabilizationTime / 60000)) + "</span> <sup class=\"units\">min.</sup></p>");
            client.println("<p><span class=\"labels\">Last read:</span> <span>" + lastReadingTime + "</span></p>");
            client.println("</div><div class=\"column\">");
            client.println("<p><i class=\"fas fa-wifi\" style=\"color:#3366ff;\"></i> <span class=\"labels\">WIFI:</span> <span>" + WiFi.SSID() + "</span></p>");
            client.println("<p><i class=\"fas fa-wifi\" style=\"color:#3366ff;\"></i> <span class=\"labels\">MQTT:</span> <span>" + mqttStateMsg + "</span></p>");

            client.println("</div></div>");

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
        { // if you got anything else but a carriage return character,
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
