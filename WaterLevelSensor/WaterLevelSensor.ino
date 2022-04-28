// ---------------------------------------------------------------------------
// JSN-SR04T ultrasonic distance sensor
// ---------------------------------------------------------------------------

/*
 * this will measure the distance between the top of the tank
 * and the top of the water.  Readings will be in inches.
 * 
 * sensor anomolies:
 *     - mechanical minimum read of sensor is 9".  Sensor limitation can cause tank overflow.
 *     - sensor has chance of reading "0".  not sure why
 *     
 *  Process: 
 *    1) Will read level every minute;
 *      1a) readings of "0" will be ignored.
 *      1b) Minmum distance will be set at "10" inches
 *    2) will convert inch readings into % full
 *    3) Will store readings in an array to get average over 15 minutes/readings
 *    4) Will upload current running average every 15 minutes to database
 *    5) [future] Send text if tank level is below a pre-defined limit
 * 
 * 
 */

 
  // NETWORK CLIENT Config
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClient.h>
    const char* ssid     = "serenity";
    const char* password = "8r0wnc0at5";
    String MACAddy;  //network MAC address
    String SensorIP; //network IP address (DHCP)

 // Onboard WEB SERVER Config
    #include <ESP8266WebServer.h>
    ESP8266WebServer server;
    String html;  //variable used for output content to web client

  // Define Trig and Echo pin:
    #define trigPin 2   // for ESP8266 - physical pin D4 connected to JSN-SR04T "dx trig" pin
    #define echoPin 15  // for ESP8266 - physical pin D8 connected to JSN-SR04T "dx echo" pin
  
  // Define variables for sensor:
    float duration;               //Sensor pulse
    float distance;               //Sensor pulse
    float tankLevel;              //the % of the tank level
  
  //Variables to set specific to each tank
    float tankFull = 10;          //minimum distance (inches) between sensor and water level aka FULL
    float tankEmpty = 21;         //maximum distance (inches) between sensor and water level aka EMPTY
    float tankLevelMax = 100;     //highest value (%) allowed for tank level
    float tankLevelMin = 0;       //lowest value (%) allowed for tank level
    float distanceOffset = 1.0;   //compensates for reading in INCHES - not sure if this is common across all sensors
    float tankAlarm = 30;         //Threshold for alerting if level falls below this %
    
    const float speedOfSound = 0.0135;  //speed of sound: inches per MICROsecond
    float durationTimeout = (tankEmpty*1.5*2)/speedOfSound;  //used to shorten wait time for pulse 
  
  // Define variables for smoothing array:
    unsigned long readingTimestamp = 0;    //timestamp [milliseconds] variable for reading cycle
    int readingDelayMS = 60000;  //time gap between readings; default is 60 seconds
    int readingDelayAdjustment;  //me being WAY too anal retentive
    const int numReadings = 15;  //number of readings to be averaged/smoothed
    int readingCount=0;          //current total number of readings
    int readings[numReadings];   //array that stores readings
    int readIndex = 0;           

    float readingSumTotal = 0;
    float readingAverage = 0;

  //Optional: Display uptime in human friendly format
    const int constSeconds = 1000;  //ms per Second
    const int constMinutes = 60000; //ms per Minute
    const int constHours = 3600000; //ms per Hour
    const int constDays = 86400000; //ms per Day
    int tsSeconds = 0;
    int tsMinutes = 0;
    int tsHours = 0;
    int tsDays = 0;
    
    String tsUptime="";     //string used to compile the uptime
  

  
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println ("Booting Up.....");
  
  // Define inputs and outputs
    pinMode(trigPin, OUTPUT);  //JSN-SR04T: dx trig
    pinMode(echoPin, INPUT);   //JSN-SR04T: tx echo 
  
  //NETWORK CLIENT setup
      WiFi.begin(ssid, password);
      Serial.println("Connecting");
      while(WiFi.status() != WL_CONNECTED) { 
        delay(500);
        Serial.print(".");
      }
        delay(15000);
      MACAddy = WiFi.macAddress();
      SensorIP = WiFi.localIP().toString();
      
      Serial.println("");
      Serial.print("Connected to WiFi network - IP Address: ");
        Serial.println(SensorIP);
      Serial.print("MAC Address: ");
      Serial.println(MACAddy);

  //WEB SERVER (on demand) setup
    server.on("/", handleRoot);   //default web browser request gets directed to "handleRoot"
    server.begin();
    Serial.println();
    Serial.println("....STARTING HTTP SERVER....");
    Serial.println();
      
}

void loop() {
  //captures HTTP request
  server.handleClient();
  
  // Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  
  if (millis()-readingTimestamp >= readingDelayMS){  // will execute only after pre-defined time period
    
    getReading();

  }
  
}

void getReading(){
   // Trigger the sensor by setting the trigPin high for 10 microseconds:
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);  //
   
      duration = pulseIn(echoPin, HIGH, durationTimeout); // Read the echoPin for round-trip pulse time.

      if (duration > 10){  //sometimes echo fails; returns zero
        readingTimestamp=millis();    //timestamp this loop
        readingDelayAdjustment= readingTimestamp % readingDelayMS;
        readingTimestamp -= readingDelayAdjustment;
     
       // Calculate the distance 
          distance = duration*speedOfSound;     //converts the duration to INCHES
          distance /= 2;                //halves distance for round trip
          distance += distanceOffset; //adds compensation to measurement
       
      // converts distance to % of tank    
          tankLevel = round(100*(1-((distance-tankFull)/(tankEmpty-tankFull))));
          tankLevel = max(tankLevel, tankLevelMin);   //doesn't allow reading to go below 0
          tankLevel = min(tankLevel, tankLevelMax);   //doesn't allow reading to go above 100
      
      // Call the smoothing formula
          smoothData();
          
      // display output
          serialMonitorOutput();
       
     } else {   
        readingTimestamp=millis()-readingDelayMS;     //if distance returns a "0", re-run reading
        Serial.print (readingTimestamp);
        Serial.println(" : Error: zero value returned, redoing reading....");
     }
      
}

void serialMonitorOutput(){   //Displays the information to Serial Monitor
      millisToHuman();
      
      Serial.print ("Sensor Timestamp (ms): ");
       Serial.println (readingTimestamp);
     
     if (tankLevel<=tankAlarm){
        Serial.println ("  !!!ALERT!!!    Low Tank Level    !!!ALERT!!!");
      }

      Serial.print("\t IP Address:\t\t\t");
        Serial.println(SensorIP);
      Serial.print("\t MAC Address:\t\t\t");
      Serial.println(MACAddy);


      
      Serial.print ("\t Uptime (D:H:M:S):\t\t");
       Serial.println (tsUptime);
       
      Serial.print ("\t Pulse duration (round trip):\t");
      Serial.print (duration,0);
      Serial.println (" µs");
      
      Serial.print("\t Distance to liquid surface:\t");
        Serial.print(distance,1);
        Serial.println (" in");
     
      Serial.print ("\t Tank level - Current:\t\t");
        Serial.print(tankLevel,1);
        Serial.println("%");

      Serial.print ("\t Tank level - Running Avg:\t");
        Serial.print(readingAverage,1);
        Serial.print("%");
        if (readingCount < numReadings) {
          Serial.print("**");     //visual flag to indicate that current average is not a full set of data yet
        }
        Serial.println();
        
      Serial.println();
}

void smoothData(){
    readingSumTotal -= readings[readIndex]; // subtract the last reading from the total
    readings[readIndex] = tankLevel;        // assigns current level to array
    readingSumTotal += readings[readIndex]; // add the currentreading to the total
    
    readIndex = readIndex + 1;              // advance to the next position in the array:
    if (readIndex >= numReadings) {         // if at the end of the array, wrap around to the beginning
      readIndex = 0;
    }
    if (readingCount < numReadings){        //accomodates initial lack of data until array is fully populated 
      readingCount++;
      readingAverage = readingSumTotal / readingCount;
                                          
    } else {
      readingAverage = readingSumTotal / numReadings;
    }
}

void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void millisToHuman(){
  tsUptime="";
  
  tsDays=readingTimestamp/constDays;
  tsHours = (readingTimestamp%constDays)/constHours;
  tsMinutes = ((readingTimestamp%constDays)%constHours)/constMinutes;
  tsSeconds = (((readingTimestamp%constDays)%constHours)%constMinutes)/constSeconds;

  if (tsDays<10){tsUptime+="0";}          //adds leading zero if needed
  tsUptime +=String(tsDays);
  tsUptime +=":";
  
    if (tsHours<10){tsUptime+="0";}       //adds leading zero if needed
    tsUptime +=String(tsHours);
    tsUptime +=":";
  
      if (tsMinutes<10){tsUptime+="0";}   //adds leading zero if needed
      tsUptime +=String(tsMinutes);
      tsUptime +=":";
  
        if (tsSeconds<10){tsUptime+="0";} //adds leading zero if needed
        tsUptime +=String(tsSeconds);
  
}

void handleRoot() {
      //this should replicated what is displyed on Serial Monitor
      
      millisToHuman();
   
   //user interface HTML code----------------
      html = "<!DOCTYPE html><html><head>";
      html += "<title>Water Tank Level Sensor</title>";
      html += "<meta http-equiv='refresh' content='60'></head><body>";
      html += "    <meta http-equiv='cache-control' content='max-age=0' />";
      html += "    <meta http-equiv='cache-control' content='no-cache' />";
      html += "    <meta http-equiv='expires' content='0' />";
      html += "    <meta http-equiv='expires' content='Tue, 01 Jan 1980 1:00:00 GMT' />";
      html += "    <meta http-equiv='Pragma' content='no-cache'>";
      html += "<h1>Water Tank Level Sensor</h1>";
      html += "<p><small>Note: Reading updates every 60 seconds</small></p>";
      if (tankLevel<=tankAlarm){
        html += "<p style='color:red'><big>  !!!ALERT!!!    Low Tank Level    !!!ALERT!!! </big></p>";
      }

      html +="<table>";
        html +="<tr>";
          html += "<td style='text-align:right'>";
            html += "Sensor Timestamp (ms):";
          html += "</td><td>";
            html += readingTimestamp;
          html += "</td></tr>";
     
          html += "<tr><td style='text-align:right'>";
            html += "IP Address:";
          html += "</td><td>";
            html += SensorIP;
          html += "</td></tr>";

          html += "<tr><td style='text-align:right'>";
            html += "MAC Address:";
          html += "</td><td>";
            html += MACAddy;
          html += "</td></tr>";
   
          html += "<tr><td style='text-align:right'>";
            html += "Uptime (D:H:M:S):";
          html += "</td><td>"; 
            html += tsUptime;
          html += "</td></tr>";

          html += "<tr><td style='text-align:right'>";
            html += "Pulse duration (round trip):";
          html += "</td><td>";
            html += duration;
            html += " &micros";
          html += "</td></tr>";

          html += "<tr><td style='text-align:right'>";
            html += "Distance to liquid surface:";
          html += "</td><td>";
            html +=distance;
            html += "in";
          html += "</td></tr>";

          html += "<tr><td style='text-align:right'>";
            html += "Current tank reading:";
          html += "</td><td>";
            html += tankLevel;
            html += "%";
          html += "</td></tr>";

          html += "<tr><td style='text-align:right'>";
            html += "Average of tank reading:";
          html += "</td><td>";
            html +=readingAverage;
            html += "%";
              if (readingCount < numReadings) {
                html += "**";     //visual flag to indicate that current average is not a full set of data yet
              }
        html += "</td></tr>";

     html +="</table>";
     html +="</body></html>";
     
     server.send(200,"text/html", html);
}
