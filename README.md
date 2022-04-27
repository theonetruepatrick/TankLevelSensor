# Arduino TankLevelSensor

Written to be used for hydroponic garden tank monitoring.  

Arduino code for using a JSN-SR04T ultrasonic distance sensor and ESP8266 to measure the liquid level in a storage tank.  This is first version that only displays results in the Serial Monitor.

Features:
  * output in Inches (not CM)
  * output shows % of tank capacity 
  * variables to set: Full level, Empty level, and Alert (if level falls below defined %)
  * Sensor takes a reading every 60 seconds (customizable)
  * Sensor data is averaged over 15 readings (customizable)
  * displays uptime in DD:HH:MM:SS as well as milliseconds.
  * code does NOT use any pre-built libraries
  * compensates for limitations of JSN-SR04T sensor
  * chocked full of comments to help modify/improve



 
