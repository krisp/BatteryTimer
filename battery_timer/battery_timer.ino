#if 1 // fix arduino ide bug
__asm volatile ("nop");
#endif

// Battery Timer sketch for Arduino Micro/Battery Timer PCB (c) 2014 David McKenzie 

#include "LowPower.h"
#include <avr/power.h>

//#define DEBUG // status on serial console (~3kb)
//#define CLI   // include CLI support

const int VOLTAGE_PIN = A3; // voltage divider is connected here
const int RELAY_PIN   = 2;  // relay driver transistor base is connected here

// initial values, cli configurable
int   OFF_WAIT    = 120;   // 2 minute timeout
int   PD_WAIT     = 45;    // 45 second wait until initial powerdown (when enabled)
float VDIV_CONST  = 3;     // R2 = 200k, R1 = 100k
float VOLTAGE_LOW = 11.5;  // immediately cut relay below this voltage
float VOLTAGE_ON  = 13.5;  // engage relay at this voltage

// states
bool relayState = false;
bool powerDown = true;
bool delaySwitch = false;
bool initialState = true;

unsigned long time = 0; // we can use millis because we only care about it when not in low power mode

void setup()
{
#ifdef DEBUG || CLI
  Serial.begin(9600);
#endif
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // make sure relay is off state
  power_spi_disable();
  power_twi_disable();
}

void loop()
{
#ifdef DEBUG
  unsigned long nowm = micros();
#endif
  unsigned long now = millis();

  if(powerDown && !relayState && !initialState) {
    // disable usb
    power_usb_disable();
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_ON);
  }  
  
  if(delaySwitch && relayState) {
    // check if delayed off has expired    
    if(now > (time + (1000 * (unsigned long)OFF_WAIT))) {
      // shut that whole thing down
      relayOff();
    }
  }
  
  // read the analog pin and convert to volts
  float volts = ((float)(analogRead(VOLTAGE_PIN) * 5) / 1024.0) * VDIV_CONST;    

  if(volts >= VOLTAGE_ON) { 
    if(!relayState)  // charging system is on, relay on
      relayOn();
    if(delaySwitch)  // clear delay switch 
      delaySwitch = false;
  } else if(volts >= VOLTAGE_LOW && volts < VOLTAGE_ON) {
    // charging system off, check if relay is on and start the timer if it hasn't already been started.
    if(relayState && !delaySwitch) {  
      time = now;
      delaySwitch = true;
    }
  } else if(volts < VOLTAGE_LOW) {
    // low voltage, turn the relay off now
    relayOff();
  }

  if(initialState && (now > (1000 * (unsigned long)PD_WAIT)))
  {
    initialState = false;
    if(!relayState)
      powerDown = true;
  }

#ifdef CLI
// TODO: check serial buffer for commands
#endif

   if(!powerDown || initialState) // wait for 500ms if we aren't powering down
   {
#ifdef DEBUG
     printStatus(nowm, volts);
#endif 
     delay(500);
   }
}

void relayOn()
{
  digitalWrite(RELAY_PIN, HIGH);
  powerDown = false;
  power_usb_enable();; // turn usb back on in case we need to reprogram
  relayState = true;
  delaySwitch = false; // if we were in a powerdown delay we should clear it 
}

void relayOff()
{
  digitalWrite(RELAY_PIN, LOW);
  relayState = false; 
  delaySwitch = false;
  time = 0;
  powerDown = true; 
}

#ifdef DEBUG
void printStatus(unsigned long now, float volts)
{
  char buf1[8];
  char buf2[128];
  dtostrf(volts, 4, 2, buf1);
  snprintf(buf2, sizeof(buf2), "%lu %lu relay: %d, delay: %d, i: %d, pD: %d, V: %s\n\r",
              now, micros()-now, relayState, delaySwitch, initialState, powerDown, buf1);
  Serial.print(buf2);
}
#endif

#ifdef CLI
//do serial stuff
#endif

