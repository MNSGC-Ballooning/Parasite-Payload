void initGPS(){
    //initiate GPS
  UBLOX_SERIAL.begin(UBLOX_BAUD);
  GPS.init();
  //Initiate GPS Data lines
  Serial.println("GPS begin");
  delay(50);
  if(GPS.setAirborne()){
    Serial.println("Airbrone mode set!");
  }

  //GPS setup and config
  Serial.println("GPS configured");
}

void updateGPS(){
  Control_Altitude = GPS.getAlt_feet();
  ascent_rate = ((Control_Altitude - prev_Control_Altitude)/(millis() - prev_time))*1000;
  prev_time = millis(); 
  prev_Control_Altitude = Control_Altitude; 
  GPSdata = String(GPS.getLat(), 4) + "," + String(GPS.getLon(), 4) + "," 
  + String(Control_Altitude, 1) + ","
  + String(GPS.getMonth()) + "/" + String(GPS.getDay()) + "/" + String(GPS.getYear()) + ","
  + String(GPS.getHour()) + ":" + String(GPS.getMinute()) + ":" + String(GPS.getSecond()) + ","
  + String(GPS.getSats()) + ",";
  
  //GPS should update once per second, if data is more than 2 seconds old, fix was likely lost
  if(GPS.getFixAge() > 4000){
    GPSdata += "No Fix,";
    //fixU == false;
  }
  else{
    GPSdata += "Fix,";
    //fixU == true;
  }

}

void ascentRateUpdate(){
    Control_Altitude = GPS.getAlt_feet();       // altitude equals the alitude recorded by the Ublox
    
    ascent_rate = (((Control_Altitude - prev_Control_Altitude)/(millis() - prev_time))) * 1000; // calculates ascent rate in ft/sec if GPS has a lock
    prev_time = millis(); 
    prev_Control_Altitude = Control_Altitude;      //Only used when determining appropiate range to use data from barometer library for altitude
}
