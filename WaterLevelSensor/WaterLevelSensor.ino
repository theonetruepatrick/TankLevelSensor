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
 * 
 * 
 * 
 * 
 */

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
    int readingTimestamp = 0;    //timestamp [milliseconds] variable for reading cycle
    int readingDelayMS = 60000;  //time gap between readings; default is 60 seconds
    int readingDelayAdjustment;  //me being WAY too anal retentive
    const int numReadings = 15;  //number of readings to be averaged/smoothed
    int readingCount=0;          //current total number of readings
    int readings[numReadings];   //array that stores readings
    int readIndex = 0;           

    float readingSumTotal = 0;
    float readingAverage = 0;

  

  
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println ("Booting Up.....");
  
  // Define inputs and outputs
    pinMode(trigPin, OUTPUT);  //JSN-SR04T: dx trig
    pinMode(echoPin, INPUT);   //JSN-SR04T: tx echo 

}

void loop() {
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

void serialMonitorOutput(){
      // Print the distance on the Serial Monitor (Ctrl+Shift+M):
      
      Serial.println (millis());
      if (tankLevel<=tankAlarm){
        Serial.println ("  !!!ALERT!!!    Low Tank Level    !!!ALERT!!!");
      }
      Serial.print ("     Pulse duration (round trip) ");
      Serial.println (duration);
      
      Serial.print("     Distance to liquid surface: ");
      Serial.print(distance);
      Serial.println (" inch");
     
      Serial.print ("     Current tank reading: ");
      Serial.print(tankLevel);
      Serial.println("%");

      Serial.print ("     Average of tank reading: ");
      Serial.print(readingAverage);
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
