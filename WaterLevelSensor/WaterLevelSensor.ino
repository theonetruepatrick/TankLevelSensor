// ---------------------------------------------------------------------------
// JSN-SR04T ultrasonic distance sensor
// ---------------------------------------------------------------------------

/*
 * this will measure the distance between the top of the tank
 * and the top of the water.  Readings will be in inches.
 * 
 * sensor anomolies:
 *     - mechanical minimum read of sensor is 9".  Sensor limitation can cause tank overflow.
 *     - sensor has 50% chance of reading "0".  not sure why
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
    float tankFull = 10;          //minimum distance (inches) between sensor and water level aka FULL
    float tankEmpty = 21;         //maximum distance (inches) between sensor and water level aka EMPTY
    float tankAlarm = 30;         //Threshold for alerting if level falls below this %
    float tankLevel = 0;          //the % of the tank level
    float tankLevelMax = 100;     //highest value (%) allowed for tank level
    float tankLevelMin = 0;       //lowest value (%) allowed for tank level
    float distanceOffset = 1;     //compensates for reading in INCHES - not sure if this is common across all sensors
    
    const float speedOfSound = 0.0135;  //speed of sound: inches per MICROsecond
    float durationTimeout = (tankEmpty*1.5*2)/speedOfSound;  //used to shorten wait time for pulse 
  
  // Define variables for smoothing array:
    int readingTimestamp = 0;    //timestamp [milliseconds] variable for reading cycle
    int readingDelayMS = 60000;  //time gap between readings
    int readingDelayAdjustment;  //me being WAY too anal retentive
    const int numReadings = 15;  //number of readings to be averaged/smoothed
    int readings[numReadings];   //array that stores readings
    int readIndex = 0;           
  
void setup() {
     Serial.begin(115200);
  // Define inputs and outputs
    pinMode(trigPin, OUTPUT);  //JSN-SR04T: dx trig
    pinMode(echoPin, INPUT);   //JSN-SR04T: tx echo 
  
  // initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readings[thisReading] = 0;
    }

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
      digitalWrite(trigPin, LOW);
   
   // Read the echoPin. pulseIn(pin, value, timeout) returns the duration (length of the pulse) in microseconds:
               //
      duration = pulseIn(echoPin, HIGH, durationTimeout);       //timeout equates to approx 70inch round trip - 

      if (duration > 1){
        readingTimestamp=millis();    //timestamp this loop
        readingDelayAdjustment %= readingDelayMS;
        readingTimestamp -= readingDelayAdjustment;
      
      
       // Calculate the distance 
          
          distance = duration*speedOfSound;     //converts the duration to INCHES
          distance = distance/2;                //halves distance for round trip
          distance = distance + distanceOffset; //adds compensation to measurement
          
      
      // converts distance to % of tank    
          tankLevel = round(100*(1-((distance-tankFull)/(tankEmpty-tankFull))));
          tankLevel = max(tankLevel, tankLevelMin);   //doesn't allow reading to go below 0
          tankLevel = min(tankLevel, tankLevelMax);   //doesn't allow reading to go above 100
      
      // display output
          serialMonitorOutput();
       
     } else {
        readingTimestamp=millis()-readingDelayMS;     //if distance returns a "0", re-run reading
        Serial.println("zero value returned, redoing reading....");
     }
      
}

void serialMonitorOutput(){
      // Print the distance on the Serial Monitor (Ctrl+Shift+M):
      Serial.print (millis());
      Serial.print (" : Pulse duration (round trip) ");
      Serial.print (duration);
      Serial.print (" : ");
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.print (" in : Tank Fill level: ");
      Serial.print(tankLevel);
      Serial.print(" %");
      if (tankLevel<=tankAlarm){
        Serial.print ("  !!!ALERT!!!    Low Tank Level    !!!ALERT!!!");
      }
      Serial.println();
}
