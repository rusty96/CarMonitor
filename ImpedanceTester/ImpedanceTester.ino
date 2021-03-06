#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// how many milliseconds between grabbing data and logging it. 1000 ms is once a second
#define LOG_INTERVAL  1000 // mills between entries (reduce to take more/faster data)

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()

String LOGFILE_HEADER = "millis,stamp,datetime,light,temp,vcc";

RTC_DS1307 RTC; // define the Real Time Clock object


// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);

  while(1);
}
// Sensor reader
int analogPins[6]= {0,1,2,3,4,5};
int raw[6]= {0};
int Vin= 5;
float Vout[6]= {0};
float R1[6]= {10000,10000,10000,10000,10000,10000};
float R2[6]= {0};
float buffer[6]= {0};

int timeInterval = 1000;//in msec
int timeSinceStart = 0;


void setup() {
  Serial.begin(9600);
  Serial.println();

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  

  logfile.println(LOGFILE_HEADER);    
#if ECHO_TO_SERIAL
  Serial.println(LOGFILE_HEADER);
#endif //ECHO_TO_SERIAL
 
  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}

void readAnalogs(bool outputIt){
  if (outputIt) Serial.print("Time ");Serial.print(timeSinceStart);

  //Read in all 6 inputs, no way to read multiple at once, 
  // so do it one at a time
  for (int i=0;i<6;i++){
    raw[i]=analogRead(analogPins[i]);
    if(raw[i]) {
      buffer[i]= raw[i] * Vin;
      Vout[i]= (buffer[i])/1024.0;
      buffer[i]= (Vin/Vout[i]) -1;
      R2[i]= R1[i] * buffer[i];
      if (outputIt){
        Serial.print(" Vout[");Serial.print(i);Serial.print("] = ");
        Serial.print(Vout[i]);
        Serial.print(" R2[");Serial.print(i);Serial.print("] = ");
        Serial.print(R2[i]);
      }
    } 
  }
  if (outputIt) Serial.print("\n");
}

void loop() {
  readAnalogs(true);
  delay(timeInterval);
  timeSinceStart++;
}
