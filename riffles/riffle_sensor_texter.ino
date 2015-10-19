#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include <JeeLib.h>

/*

Riffle-Tweeter 
by John Keefe
September 2015

*  This code takes the conductivity and temperature of 
*  water and texts it to a table at data.sparkfun.com
*  by way of Twilio and a node.js server
*  
*  The hardware here is 
*  - a Riffle board v0.96
*  - a Riffle conductivity/temperature daughter board
*  - An Adafruit Fona board
*  
*  More details on the project at:
*  http://streamlab.cc and
*  http://johnkeefe.net and a related article:
*  http://johnkeefe.net/make-every-week-temperature-tweeter
*  
*  More information on Riffle, and the basis for some of this code
*  by Don Blair, is at:
*  http://github.com/openwaterprojet
*  
*  Built on code by Rob Tillarart and Lady Ada/Adafruit
*  
*  library for fona
*  https://learn.adafruit.com/adafruit-fona-mini-gsm-gprs-cellular-phone-module/arduino-test
*  
*  turnOnFONA and turnOffFONA functions by Kina Smith
*  http://www.kinasmith.com/spring2015/towersofpower/fona-code/
*  
*  low-power sleep code from Jean-Claude Wippler
*  http://jeelabs.net/pub/docs/jeelib/classSleepy.html and
*  http://jeelabs.net/projects/jeelib/wiki
*  Which is well-described here: http://jeelabs.org/2011/12/13/developing-a-low-power-sketch/
*  
*  The sleep code messes with the serial connection, so serial functions only operate
*  when "debug" is set to true.
*  
*  Additionally ... and this is a bit of a pain ... I need to use D0 and D1 pins on
*  the Riffle for the Fona, but those pins are also the RX & TX for the Riffle brain.
*  So I have to *disconnect* those wires when uploading to code to the board.

Comments by Adafruit for the FONA phone module:
*  
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA 
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are 
  required to interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution

  THIS CODE IS STILL IN PROGRESS!

  Open up the serial console on the Arduino at 115200 baud to interact with FONA

  Note that if you need to set a GPRS APN, username, and password scroll down to
  the commented section below at the end of the setup() function.

*/

// general variables
boolean debug = false;
#define LED 9 // Riffle's onboad LED is 9

//// fona variables

//      FONA_VIO - wire to Riffle 3V3, which is also on the daughter board
#define FONA_RX 1 // communications - wire to Riffle D1
#define FONA_TX 0 // communications - wire to Riffle D0
#define FONA_KEY 13 // pulse to power up or down - wire to Riffle D13
#define FONA_PS 11 // status pin - wire to Riffle D11
#define FONA_RST 12 // the reset pin - wire to Riffle 12
//      FONA_GND - wire to Riffle Ground, which is also on the daughter board

int keyTime = 2000; // Time needed to turn on/off the Fona
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint16_t vbat;
String phone_number = "insert_phone_number_here"; // put receiving number here
String base_message;
char sendto[21], message[141];

//// sensor variables

// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 9550
// how long to sleep between measurements
#define sleepSeconds 3

int temp_sample;
float temp_average;
float steinhart;
int interruptPin = 1; //corresponds to D2

// conductivity variables
long pulseCount = 0;  //a pulse counter variable
unsigned long pulseTime,lastTime, duration, totalDuration;
int samplingPeriod=3; // the number of seconds to measure 555 oscillations
int sensorBoard = 8; // the pin that powers the 555 subcircuit
int sensor_data;

//// sleep variables

// must be defined in case we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
// minutes between texts
int mins_between_texts = 10; 

void setup()
{

  // fona setup
    
    // Set FONA pins. Actually pretty important to do FIRST, 
    // otherwise the board can be inconsistently powered during startup. 
    pinMode(FONA_PS, INPUT); 
    pinMode(FONA_KEY,OUTPUT);   
    digitalWrite(FONA_KEY, HIGH);
    
    // make communications with fona slow so it is easy to read
    fonaSS.begin(4800);
    turnOnFONA(); // Power up FONA

  // sensor setup

    pinMode(sensorBoard,OUTPUT); //turns on the 555 timer and thermistor subcircuit
    digitalWrite(sensorBoard,LOW); //turns on the 555 timer and thermistor subcircuit

  // general setup

    pinMode(LED, OUTPUT); // set up Riffle LED
    digitalWrite(LED, LOW); // set turn off Riffle LED

    // initialize serial if in debug mode
    if (debug) Serial.begin(9600);

}

void loop()
{
  //// READ THE SENSOR DATA
  
  //// conductivity 
  
  pulseCount=0; //reset the pulse counter
  totalDuration=0;  //reset the totalDuration of all pulses measured

  // attach an interrupt counter to interrupt pin 1 (digital pin #3)
  // the only other possible pin on the 328p is 
  // interrupt pin #0 (digital pin #2)
  attachInterrupt(interruptPin,onPulse,RISING); 

  // start the stopwatch
  pulseTime=micros(); 

  // give ourselves samplingPeriod seconds to make this measurement,
  // during which the "onPulse" function will count up all the pulses, 
  // and sum the total time they took as 'totalDuration' 
  delay(samplingPeriod*1000); 

  // we've finished sampling, so detach the interrupt function
  // -- don't count any more pulses
  detachInterrupt(interruptPin); 

  // calculate the frequency detected by the sensor from pulseCount & duration
  float freqHertz;
  
  if (pulseCount>0) {
    // calculate the total duration, in seconds, per pulse (note that totalDuration was in microseconds)
    double durationS=(totalDuration/double(pulseCount))/1000000.; 
    freqHertz=1./durationS;
  }
  else {
    freqHertz=0.;
  }

  // temperature
  
  // take N temperature samples in a row, with a slight delay
  temp_average = 0;
  for (int i=0; i< NUMSAMPLES; i++) {
    temp_sample = 0;
    temp_sample = analogRead(THERMISTORPIN);
    temp_average += temp_sample;
    delay(10);
  }
  temp_average /= NUMSAMPLES;
 
  // convert the value to resistance
  temp_average = 1023 / temp_average - 1;
  temp_average = SERIESRESISTOR / temp_average;

  // calcluate the temperature
  steinhart = temp_average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;   
  
  // Power up FONA
  turnOnFONA();
  
  // read the fona battery level  
  getFonaBatt();

  // COMPOSE MESSAGE TO SEND 

  // Note: I'm converting floats like freqHertz to Strings
  // using dtostrf as defined here: http://forum.arduino.cc/index.php?topic=126618.0
  //   dtostrf(YourNumber, TotalStringLength, NumberOfDecimals, TheTargetArray)
  //   TotalStringLength    > Must include all characters the '.' and the null terminator
  //   NumberOfDecimals  > The output is rounded .456 become .46 at 2 decimals
  //   TheTargetArray must be declared
  //   And my targed is a throwaway string called "padding"

  // building the message to text
  base_message = "RiffleBottle2,";
  char padding[10];
  base_message += dtostrf(freqHertz, 6, 2, padding);
  base_message += ",";
  base_message += dtostrf(steinhart, 5, 2, padding);
  base_message += ",";
  base_message += String(vbat); // Convert vbat int to String
  
  // turn the phone number String into a character array called sendto
  phone_number.toCharArray(sendto,20);

  // convert base_message String to a character array called message
  // for the fona software
  base_message.toCharArray(message,139);

  if (debug) Serial.println(message);

  // TEXT VIA FONA

  if (!fona.sendSMS(sendto, message)) {
    if (debug) Serial.println(F("Failed"));
  } else {
    if (debug) Serial.println(F("Sent!"));
  }

  // PAUSE UNTIL NEXT CHECK & TEXT

  // wait for the sending process to conmplete
  // note that sleepy code messes up serial, so only using in production mode
  if (!debug) {
    Sleepy::loseSomeTime(10000); // instead of delay(10000);
  } else {
    delay(10000);
  }

  // power down fona
  turnOffFONA();
  
  // Enter arduino low-power mode
  // loseSomeTime has a max of 60secs ... so looping for multiple minutes
  for (byte i = 0; i < mins_between_texts; ++i) {
    if (!debug) {
      Sleepy::loseSomeTime(60000); // sleep a minute
    } else {
      delay(1000); // delay a second in debug mode 
    }
  }

}

// This turns FONA ON
void turnOnFONA() { 
    if(! digitalRead(FONA_PS)) { //Check if it's On already. LOW is off, HIGH is ON.
        // Serial.print("FONA was OFF, Powering ON: ");
        digitalWrite(FONA_KEY,LOW); //pull down power set pin
        unsigned long KeyPress = millis(); 
        while(KeyPress + keyTime >= millis()) {} //wait two seconds
        digitalWrite(FONA_KEY,HIGH); //pull it back up again
        // Serial.println("FONA Powered Up");
        // Serial.println("Initializing please wait 20sec...");
        // delay for 20sec to establish cell network handshake.
        // Sleepy::loseSomeTime(20000); 
        delay(20000);
    } else {
        // Serial.println("FONA Already On, Did Nothing");
    }

    // test to make sure all is well & FONA is responding
    // this also appears ke to reestablishing the software serial
    if (! fona.begin(fonaSS)) {
      // Serial.println(F("Couldn't find FONA"));
    } else {
      // Serial.println(F("FONA is OK"));
    }
}

// This does the opposite of turning the FONA ON (ie. OFF)
void turnOffFONA() { 
    if(digitalRead(FONA_PS)) { //check if FONA is OFF
        Serial.print("FONA was ON, Powering OFF: "); 
        digitalWrite(FONA_KEY,LOW);
        unsigned long KeyPress = millis();
        while(KeyPress + keyTime >= millis()) {}
        digitalWrite(FONA_KEY,HIGH);
        Serial.println("FONA is Powered Down");
    } else {
        Serial.println("FONA is already off, did nothing.");
    }
}

// This is the pulse counter for the conductivity sensor
void onPulse()
{
  pulseCount++;
  // if (debug) Serial.print("pulsecount=");
  // if (debug) Serial.println(pulseCount);
  lastTime = pulseTime;
  pulseTime = micros();
  duration=pulseTime-lastTime;
  totalDuration+=duration;
  // if (debug) Serial.println(totalDuration);
}

// Get the fona battery and set it to vbat global
void getFonaBatt() {

  if (! fona.getBattPercent(&vbat)) {
    if (debug) Serial.println("Couldn't read Fona battery");
  } else {
    if (debug) Serial.print(F("Battery: ")); 
    if (debug) Serial.print(vbat); 
    if (debug) Serial.println(F("%"));
  }
  
}

