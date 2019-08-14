//Libraries
#include <Adafruit_MAX31856.h>  //thermocouple library
#include <XBee.h>  //xbee library
#include <SD.h>  //SD card library
#include <LatchRelay.h> //Latching relay library
#include <UbloxGPS.h> //GPS library

/*
  /-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\
  \               Parasite Payload                                          /
  /               Author: Billy Straub (strau257)                           \
  \-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/-\-/


  Arduino Uno pin connections:
   -------------------------------------------------------------------------------------------------------------------------
     Component                    | Pins used               | Notes
     -                              -                         -
     SD card reader on shield     | D8  D11 D12 D13         | CS, MOSI, MISO, CLK pins in that order
     Honeywell Pressure           | A0                      |
     Adafruit Thermocouple #1     | D3  D4  D5  D6          | SCK, SDO, SDI, CS pins in that order
   -------------------------------------------------------------------------------------------------------------------------
*/



//SD shield pin definition (Arduino shield is pin 4, Sparkfun shield is pin 8, Adafruit shield is pin 10)
#define chipSelect 8

//APRS Macros
#define APRS_ON 22
#define APRS_OFF 23
#define APRS_TIMER 14400000
#define LOG_TIMER 7000

//GPS Macros
#define UBLOX_SERIAL Serial1
#define UBLOX_BAUD 4800 

//Plantower
#define PLAN_SERIAL Serial2
//Plantower Definitions
  String dataLog = "";                                  //Used for data logging
  int nhits=1;                                          //Used to count successful data transmissions    
  int ntot=1;                                           //Used to count total attempted transmissions
  bool goodLog = false;                                 //Used to check if data is actually being logged
  int badLog = 0;                                       //Counts the number of failed log attempts
  //String filename = "ptLog.csv";                      //File name that data wil be written to
  //File ptLog;                                         //File that data is written to 
                  
  struct pms5003data {
    uint16_t framelen;
    uint16_t pm10_standard, pm25_standard, pm100_standard;
    uint16_t pm10_env, pm25_env, pm100_env;
    uint16_t particles_03um, particles_05um, particles_10um;
    uint16_t particles_25um, particles_50um, particles_100um;
    uint16_t unused;
    uint16_t checksum;
  } planData;                                           //This struct will organize the plantower bins into usable data

//SD file logging 
File datalog;                     //File object for datalogging
char filename[] = "Paras00.csv";   //Template for file name to save data
bool SDactive = false;            //Used to check for SD card before attempting to log data

//GPS
UbloxGPS GPS(&UBLOX_SERIAL);
//boolean fixU = false;
float alt_GPS = 0;               // altitude calculated by the GPS in feet
float Control_Altitude = 0;
float prev_Control_Altitude = 0;
unsigned long prev_time = 0;
float ascent_rate = 0;
String GPSdata = "";
static byte Descent_Counter = 0;



//Thermocouple object
 Adafruit_MAX31856 maxthermo1 = Adafruit_MAX31856(6, 5, 4, 3);


//Honeywell Analog Pressure Sensor Values:
int pressureSensor = 0;
float pressureSensorV = 0;
float psi = 0;
float atm = 0;


unsigned long time;     //Used to keep time running

LatchRelay APRS(APRS_ON, APRS_OFF);   //APRS relay object
unsigned long loopTime = 0;           //Loop timer


void setup() {

  Serial.begin(9600);
  initGPS();

  //Relay Setup
  APRS.init(0);                                              //APRS Relay initialized and turned off

  //Setup Adafruit MAX_31856 Thermocouples #1-4
  maxthermo1.begin();
  maxthermo1.setThermocoupleType(MAX31856_TCTYPE_K);

  //SD card setup
  pinMode(10, OUTPUT);                                      //Needed for SD library, regardless of shield used
  pinMode(chipSelect, OUTPUT);
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect))                                //Attempt to start SD communication
    Serial.println("Card failed, not present, or voltage supply is too low.");          //Print out error if failed; remind user to check card
  else {                                                    //If successful, attempt to create file
    Serial.println("Card initialized successfully.\nCreating File...");
    for (byte i = 0; i < 100; i++) {                        //Can create up to 100 files with similar names, but numbered differently
      filename[5] = '0' + i / 10;
      filename[6] = '0' + i % 10;
      if (!SD.exists(filename)) {                           //If a given filename doesn't exist, it's available
        datalog = SD.open(filename, FILE_WRITE);            //Create file with that name
        SDactive = true;                                    //Activate SD logging since file creation was successful
        Serial.println("Logging to: " + String(filename));  //Tell user which file contains the data for this run of the program
        break;                                              //Exit the for loop now that we have a file
      }
    }
    if (!SDactive) Serial.println("No available file names; clear SD card to enable logging");
  }
  String header = "Pressure, Temp 1, Time, Lat, Long, Alt, Date, Real Time, Satellites, hits, millis, 3, 5, 10, 25, 50, 100";
  Serial.println(header);
  if (SDactive) {
    datalog.println(header);
    datalog.close();
  }

//Plantower Setup
Serial2.begin(9600);
while (!Serial2) ;


  
}



void loop() {

if (millis()-loopTime >= LOG_TIMER){
  loopTime = millis();
  updateGPS();
  //Pressure Sensor
  pressureSensor = analogRead(A0);                       //Read the analog pin
  pressureSensorV = pressureSensor * (5.0 / 1024);        //Convert the digital number to voltage
  psi = (pressureSensorV - (0.1 * 5.0)) / (4.0 / 15.0);   //Convert the voltage to proper unitime
  atm = psi / 14.696;                                     //Convert psi to atm
  
  //Thermocouples 1-4 temperatures
  float T1int = maxthermo1.readThermocoupleTemperature();


  int PR;               //Creates integers that can be changed, allowing the sig figs to be changed in the later strings depending on temp/pressure.
  int TT;

  if (0<atm){           //Conditional function to detemine number of sig figs based on pressure (and temperature in lower statements) to keep columns organized.
    PR = 3;
  }
    else if (0>=atm){
      PR = 2;
    }

  if (T1int<=-10){
    TT = 2;
  }
    else if (-10<T1int<=0 || 10<=T1int){
      TT = 3;
    }
    else if (0<T1int<10){
      TT = 4;
    }


  String atmSTR = String(atm, PR);                        //Converts Pressure to a string and uses sig figs based on conditional function above.
  String T1intSTR = String(T1int, TT);
  
  time = millis() / 1000;                                 //Converts time to seconds and starts the time at zero by subtracting the intial 26 seconds.

  String data = String(atmSTR + "," + T1intSTR + "," + time);
  data += ',' + GPSdata + ',' + dataLog;
  Serial.println(data);
  if (SDactive) {
    datalog = SD.open(filename, FILE_WRITE);
    datalog.println(data);                                //Takes serial monitor data and adds to SD card
    datalog.close();                                      //Close file afterward to ensure data is saved properly
  } 

  if (millis() >= APRS_TIMER) {
    APRS.setState(1);
  }
  else if (ascent_rate < -16.66) {
    Descent_Counter++;
    if (Descent_Counter >= 5) {
      APRS.setState(1);
      Descent_Counter = 0;
    }
  }
  else {
    Descent_Counter = 0;
  }
}
GPS.update();
readPMSdata(&PLAN_SERIAL);

}
