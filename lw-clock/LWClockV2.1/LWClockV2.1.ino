//Multifunctional clock based on ESP8266 и MAX79xxx for Home Automation (IoT)
#include "config.h"

// Declare MQTTLine as an external variable
extern String MQTTLine;

void setup() {
  Serial.begin(115200);
  FS_init(); Serial.println(F("Start FS"));   //Run FS
  loadConfig(); Serial.println(F("FileConfig"));
  Display_init(); Serial.println(F("Start screen"));
  if(myWIFI.begin(ssid.c_str(), password.c_str(), SSDP_Name.c_str(), ssidAP.c_str(), passwordAP.c_str(), apIP.c_str())) {
    Serial.println(F("Start WIFI"));  //Run WIFI
  }
  Serial.println(myWIFI.getDevStatusIP());
  if (WiFi.status() == WL_CONNECTED)  {
    SSDP_Name = myWIFI.getNameSSDP();
    SSDP_init(); Serial.println(F("Start SSDP"));   //Run SSDP 
  }
  Time_init(); Serial.println(F("Start Time"));  // Get time from NTP
  server_httpinit(); Serial.println(F("Start HTTPServer"));   //Run HTTP server
  if (WiFi.status() != WL_CONNECTED)  {
    String sText = logo + " " + WiFi.softAPIP().toString();
    sText.toCharArray(buf, 256);
    P.displayScroll(buf, PA_LEFT, PA_SCROLL_LEFT, 50);
    Serial.println(sText);
  }
  else {
    String sText = logo + " " +  WiFi.localIP().toString();
    sText.toCharArray(buf, 256);
    P.displayScroll(buf, PA_LEFT, PA_SCROLL_LEFT, 50);
    Serial.println(sText);
  }
  for (uint8_t i=0; i<ARRAY_SIZE(catalog); i++)  {
    //catalog[i].speed *= P.getSpeed();
    //catalog[i].speed *= speedTicker;
    catalog[i].pause *= 500;
  }
  init_mqtt(); Serial.println(F("Start MQTT"));
#if (USE_BME280 == true || USE_DHT == true)
  init_sensor(); Serial.println(F("Start Sensor"));
#endif   
  pinMode(ALARM_PIN, INPUT_PULLUP);
  pinMode(BUZ_PIN, OUTPUT);  
  digitalWrite(BUZ_PIN, LOW); 
  while (!P.displayAnimate()) {
    delay(100);  
  }
  if (WiFi.status() == WL_CONNECTED && (isLedWeather || isLedForecast || isLedSea))  {
    if (isLedWeather) {
      strWeather = GetWeather(); delay(1000);   
    }
    if (isLedForecast) {
      strWeatherFcast = GetWeatherForecast(); delay(1000);  
    }
    if (isLedSea) {
      strSea = GetSea();  
    }        
  }
}

uint8_t oldModeShow = 0; //****************************
float oldnowtime = 111; //****************************

void displayShowText(String message) {
  Serial.println("Displaying showtext: " + message);

  // Clear the display and set the text
  P.displayClear();
  P.displayText(message.c_str(), PA_CENTER, 5 * speedTicker, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  // Start the display animation
  P.displayAnimate();
  unsigned long startTime = millis();

  // Keep the message displayed for 10 seconds
  while (millis() - startTime < 10000) {
    if (P.displayAnimate()) {
      // Restart the animation if it finishes
      P.displayReset();
    }
    delay(1);
  }

  // Reset the display and switch back to the clock
  modeShow = 1;
  P.displayClear();
  P.displayReset();
}

void loop() {
  HTTP.handleClient();
  if ((millis() < timeCount))  { //If use device more than 50 days
    timeCount = millis(); lastTimeWeather = millis(); lastTimeWeatherFcast = millis(); lastNtpTime = millis();
    ReconnectTime = millis(); mqttDhtTime = millis(); lastTimePHT = millis(); tspeakDhtTime = millis(); timeStopAlarm = millis();
  } 
  if (statusUpdateNtpTime && (millis() - lastNtpTime >= TIME_UPDATE)) { //Update time from NTP
    lastNtpTime = millis();
    Serial.println("Update time");
    timeSynch();    
  }  
  if (!statusUpdateNtpTime && (millis() - lastNtpTime >= NTP_UPDATE)) { //When updating from NTP in the absence of a server response, we try to synchronize again
    lastNtpTime = millis();
    statusUpdateNtpTime = 1;
    Serial.println("Try sync NTP");
    timeSynch();    
  }
  workAlarms();  
#if (USE_BME280 == true || USE_DHT == true) //If we have what send to MQTT or TSPEAKE
  if(mqttOn && WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected() && (millis() - ReconnectTime > MQTT_CONNECT)) {
      reconnect();
      ReconnectTime = millis();
    }
    mqttClient.loop(); 
    if (millis() - mqttDhtTime > MQTT_SEND_INT) {
      sendMQTT();
      mqttDhtTime = millis();
      Serial.println("Send to MQTT");
    }
  }
  if(tspeakOn && WiFi.status() == WL_CONNECTED) {
    if (millis() - tspeakDhtTime > TSPEAK_SEND_INT) {
      sendTspeak();
      tspeakDhtTime = millis();
      Serial.println("Send to TSPEAK");
    }    
  }
#else //Use MQTT for get string only
  if(mqttOn && WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected() && (millis() - ReconnectTime > MQTT_CONNECT)) {
      reconnect();
      ReconnectTime = millis();
    }
    mqttClient.loop(); 
  }  
#endif
  time_t tn = now();
  int hour_now = hour(tn)+float(minute(tn))/100;
  float nowtime = hour_now + float(minute(tn)) / 100;
  if (oldnowtime != nowtime) {Serial.println("nowtime: "+ String(nowtime)); oldnowtime = nowtime;} //****************************
  if (compTimeInt(global_start, global_stop, nowtime)) { //Global check the time of display data
    if (modeShow > maxModeShow) modeShow = 1;
    if (oldModeShow != modeShow) {Serial.println("modeShow: "+ String(modeShow)); oldModeShow = modeShow; displayClockCount =millis(); displayClockFirst = true;} //****************************
    uint8_t rnd = random(0, ARRAY_SIZE(catalog));
    if (compTimeInt(dmodefrom, dmodeto, nowtime)) P.setIntensity(brightd); else P.setIntensity(brightn);
    if (modeShow == 1) { //Show time
      float nowtime = hour(tn)+float(minute(tn))/100;
      if (isLedClock && compTimeInt(clockFrom, clockTo, nowtime)) {
        //showText(GetTime(), PA_CENTER, catalog[rnd].speed*speedTicker, 5000, catalog[rnd].effect, catalog[rnd].effect);
        displayTime(false);
      if ((minute(tn) % 15 == 0) && (second(tn) == 0)) {  // Check if the minutes read 00, 15, 30, or 45}
        displayTime(true);
        modeShow++;
        P.displayReset();        
      }
    }
    else {
      modeShow++;  
    }
  }
    if (modeShow == 2) { //Show creeping line 1
      float nowtime = hour(tn)+float(minute(tn))/100; 
      //Serial.print("txtFrom0: ");Serial.print(txtFrom0);Serial.print(" nowtime: ");Serial.println(nowtime);
      // Add a check to ensure strText0 is not empty
      if (isTxtOn0 && compTimeInt(txtFrom0, txtTo0, nowtime) && !strText0.isEmpty()) {
        if (isCrLine0) {
          showText(strText0, PA_LEFT, 5 * speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        } else {
          showText(strText0, PA_CENTER, catalog[rnd].speed * speedTicker, catalog[rnd].pause * 3, catalog[rnd].effect, catalog[rnd].effect);
        }
      } else {
        modeShow++;
      }
    }
    
    if (modeShow == 3) { //Show date
      float nowtime = hour(tn)+float(minute(tn))/100;
      if (isLedDate && compTimeInt(dateFrom, dateTo, nowtime)) {
        showText(GetDate(), PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      }
      else {
        modeShow++;  
      }
    } 
  
    if (modeShow == 4) { //Show creeping line 2
      float nowtime = hour(tn)+float(minute(tn))/100; 
      //Serial.print("txtFrom[0]: ");Serial.print(txtFrom[0]);Serial.print(" nowtime: ");Serial.println(nowtime);
      if (isTxtOn1 && compTimeInt(txtFrom1, txtTo1, nowtime)) {
        if (isCrLine1) {
          showText(strText1, PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);  
        }
        else {
          showText(strText1, PA_CENTER, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);
          //showText(catalog[rnd].psz, just, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);        
        }
      }
      else {
        modeShow++;
      }
    }  
  
    if (modeShow == 5) { // Show weather
      float nowtime = hour(tn) + float(minute(tn)) / 100;
      if (isLedWeather && compTimeInt(weathFrom, weathTo, nowtime)) {
        if (millis() - lastTimeWeather > PER_GET_WEATHER) { // Avoid overloading the server
          strWeather = GetWeather();
          lastTimeWeather = millis();
        }
        showText(strWeather, PA_LEFT, 5 * speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      } else {
        modeShow++;
      }
    }
    
    if (modeShow == 6) { //Show creeping line 3
      float nowtime = hour(tn)+float(minute(tn))/100; 
      //Serial.print("txtFrom[0]: ");Serial.print(txtFrom[0]);Serial.print(" nowtime: ");Serial.println(nowtime);
      if (isTxtOn2 && compTimeInt(txtFrom2, txtTo2, nowtime)) {
        if (isCrLine2) {
          showText(strText2, PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);  
        }
        else {
          showText(strText2, PA_CENTER, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);
          //showText(catalog[rnd].psz, just, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);        
        }
      }
      else {
        modeShow++;
      }
    } 
    if (modeShow == 7) { //Show creeping line 4
     float nowtime = hour(tn)+float(minute(tn))/100; 
      //Serial.print("txtFrom[0]: ");Serial.print(txtFrom[0]);Serial.print(" nowtime: ");Serial.println(nowtime);
      if (isTxtOn3 && compTimeInt(txtFrom3, txtTo3, nowtime)) {
        if (isCrLine3) {
          showText(strText3, PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);  
        }
        else {
          showText(strText3, PA_CENTER, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);
          //showText(catalog[rnd].psz, just, catalog[rnd].speed*speedTicker, catalog[rnd].pause, catalog[rnd].effect, catalog[rnd].effect);        
        }
      }
      else {
        modeShow++;
      }
    }   
    if (modeShow == 8) { // Show weather forecast
      float nowtime = hour(tn) + float(minute(tn)) / 100;
      if (isLedForecast && compTimeInt(fcastFrom, fcastTo, nowtime)) {
        if (millis() - lastTimeWeatherFcast > PER_GET_WEATHER_FCAST) { // Avoid overloading the server
          strWeatherFcast = GetWeatherForecast();
          lastTimeWeatherFcast = millis();
        }
        showText(strWeatherFcast, PA_LEFT, 5 * speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      } else {
        modeShow++;
      }
    }
    else if (modeShow == 9) { // Show creeping line from MQTT
      float nowtime = hour(tn) + float(minute(tn)) / 100;
      if (compTimeInt(mqttFrom, mqttTo, nowtime)) { // Check if within the defined time range
        if (!MQTTLine.isEmpty()) {
          Serial.println("Displaying MQTTLine in mode 9: " + MQTTLine); // Debug output
          Serial.println("MQTTLine length: " + String(MQTTLine.length())); // Print string length

          // Calculate display duration based on string length
          int displayDuration = MQTTLine.length() * 200; // Example: 200ms per character

          // Display MQTTLine with dynamic duration
          showText(MQTTLine, PA_LEFT, 5 * speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        } else {
          Serial.println("MQTTLine is empty in mode 9, skipping display."); // Debug output
        }
      } else {
        Serial.println("Outside mqttFrom and mqttTo range, skipping mode 9 display."); // Debug output
      }
      modeShow++; // Increment modeShow after displaying
    }
    if (modeShow == 10) { //Show  SEA Temp
      float nowtime = hour(tn)+float(minute(tn))/100;
      if (isLedSea && compTimeInt(seaFrom, seaTo, nowtime)) {
        if (millis() - lastTimeSea > PER_GET_SEA) { //чтобы не грузить сервер и часы
          //Serial.print(" PER_GET_WEATHER: ");Serial.println(PER_GET_WEATHER);
          strSea = GetSea();
          lastTimeSea = millis();
        }    
        showText(strSea, PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      }
      else {
        modeShow++;  
      }    
    }            
#if (USE_BME280 == true || USE_DHT == true)
    if (modeShow == 11) { //Show Temp, hum, press
      static uint8_t modeShowUP = 1;
      static String tempUP, humUp, pressUP;
      float nowtime = hour(tn)+float(minute(tn))/100;
      if (isLedTHP&& compTimeInt(thpFrom, thpTo, nowtime)) {
        if (millis() - lastTimePHT > PER_GET_THP) { //чтобы не грузить
          #if USE_BME280 == true
            //strTHP = onboard[lang] + getTempBME280() + hum[lang] + getHumBME280() + pres[lang] + getPressBME280() + "mm";
            tempUP = getTempBME280(dataCorrect, 1)+"\xC5"; humUp = getHumBME280(dataCorrect, 0)+"%"; pressUP = getPressBME280(0)+"\xC4";
          #endif  
          Serial.println(strTHP);
          lastTimePHT = millis();
        }  
        #if USE_BME280 == true 
        //if (modeShowUP > 6) modeShowUP = 1;
        if (modeShowUP > 4) modeShowUP = 1;
        //if (modeShowUP==1) modeShowUP +=showTextUP(temperIn[lang], 5*speedTicker, 500, PA_SCROLL_UP, PA_SCROLL_UP, false); 
        if (modeShowUP==1) modeShowUP += showTextUP("Home:", 5*speedTicker, 500, PA_SCROLL_UP, PA_SCROLL_UP_LEFT, false);
        if (modeShowUP==2) modeShowUP += showTextUP(tempUP, 5*speedTicker, 1250, PA_SCROLL_UP, PA_SCROLL_UP_LEFT, false);
        //if (modeShowUP==3) modeShowUP +=showTextUP(humIn[lang], 5*speedTicker, 500, PA_SCROLL_UP, PA_SCROLL_UP, false);
        if (modeShowUP==3) modeShowUP +=showTextUP(humUp, 5*speedTicker, 1750, PA_SCROLL_UP, PA_SCROLL_UP_RIGHT, false); 
        //if (modeShowUP==5) modeShowUP +=showTextUP(presIn[lang], 5*speedTicker, 500, PA_SCROLL_UP, PA_SCROLL_UP, false);
        if (modeShowUP==4) modeShowUP +=showTextUP(pressUP, 5*speedTicker, 1750, PA_SCROLL_UP, PA_SCROLL_UP_RIGHT, true);                 
        //showText(strTHP, PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        #endif           
      }
      else {
        modeShow++;  
      }          
    }
#endif    
  }  
  else {
    if (nowtime != 0) {
      P.displayClear();       
    }
    else {
      showText(F("You use NTP server but no Wi-Fi or Internet"), PA_LEFT, 5*speedTicker, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT); 
#if USE_RTC == true
      useRTC = true; 
#endif          
    }
  }
  delay(1); 
}

