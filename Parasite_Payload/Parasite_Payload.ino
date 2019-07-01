//Libraries
#include <Adafruit_MAX31856.h>  //thermocouple library
#include <XBee.h>  //xbee library
#include <SD.h>  //SD card library

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

//SD file logging
File datalog;                     //File object for datalogging
char filename[] = "Paras00.csv";   //Template for file name to save data
bool SDactive = false;            //Used to check for SD card before attempting to log data


//Thermocouple object
 Adafruit_MAX31856 maxthermo1 = Adafruit_MAX31856(6, 5, 4, 3);


//Honeywell Analog Pressure Sensor Values:
int pressureSensor = 0;
float pressureSensorV = 0;
float psi = 0;
float atm = 0;


unsigned long time;     //Used to keep time running



void setup() {

  Serial.begin(9600);

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
  String header = "Pressure   Temp 1    Time";
  Serial.println(header);
  if (SDactive) {
    datalog.println(header);
    datalog.close();
  }
  
}



void loop() {

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

  String data = String(atmSTR + "      " + T1intSTR + "    " + time);
  Serial.println(data);
  if (SDactive) {
    datalog = SD.open(filename, FILE_WRITE);
    datalog.println(data);                                //Takes serial monitor data and adds to SD card
    datalog.close();                                      //Close file afterward to ensure data is saved properly
  }
  
  delay(1000);

}
