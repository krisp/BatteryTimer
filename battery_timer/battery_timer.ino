#if 1 // fix arduino ide bug
__asm volatile ("nop");
#endif

// Battery Timer sketch for Arduino Micro/Battery Timer PCB (c) 2014 David McKenzie 
#define DEBUG // status on serial console (~3kb)
#define CLI   // include FreakLabs CLI support

#include "LowPower.h"
#include <avr/power.h>
#include <EEPROM.h>
#include "RunningMedian.h"
#ifdef CLI
#include "Cmd.h"
#endif

struct _params_t
{
  int   CHECK;
  int   OFF_WAIT;
  int   PD_WAIT;
  int   VDIV_CONST;
  float VDROP_PCT;
  float VOLTAGE_LOW;
  float VOLTAGE_ON;
} params = {81, 120, 45, 3, 0.99, 10.8, 13.2}; // default values, changing the check byte will reset saved settings

const int VOLTAGE_PIN  = A3; // voltage divider is connected here
const int RELAY_PIN    = 2;  // relay driver transistor base is connected here
const int EEPROM_START = 8;  // start address in eeprom, changing this will reset saved settings

float volts = 0.0; // voltage now global so it is accessable from cli

RunningMedian voltmedian = RunningMedian(9); // odd size is best

// states
bool relayState = false;
bool powerDown = true;
bool delaySwitch = false;
bool initialState = true;

unsigned long time = 0; // we can use millis because we only care about it when not in low power mode
unsigned long pd_time = 0; // this allows us reset initial mode by PD_WAIT when CLI is active

void setup()
{
// check for params in eeprom
  loadParams();

#if defined(CLI) || defined(DEBUG)
  cmdInit(9600);
  cmdAdd("help", help);
  cmdAdd("status", bstatus);
  cmdAdd("list", list);
  cmdAdd("set", set);  
  cmdAdd("save", save);
  cmdAdd("sleep", sleep);
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

  // read the analog pin and convert to volts
  volts = ((float)(analogRead(VOLTAGE_PIN) * 5) / 1024.0) * params.VDIV_CONST;
  voltmedian.add(volts);
  
  if(delaySwitch && relayState) {
    // check for a voltage drop 1% lower than the median voltage and reset the delay timer (in use detection)
    if(volts < (params.VDROP_PCT*voltmedian.getMedian()))
    {
      time = now; 
#ifdef DEBUG
      Serial.println("Voltage drop detected, resetting timer");
#endif
    }
    // check if delayed off has expired    
    if(now > (time + (1000 * (unsigned long)params.OFF_WAIT))) {
      // shut that whole thing down
      relayOff();
    }    
  }
  
  if(volts >= params.VOLTAGE_ON) { 
    if(!relayState)  // charging system is on, relay on
      relayOn();
    if(delaySwitch)  // clear delay switch 
      delaySwitch = false;
  } else if(volts >= params.VOLTAGE_LOW && volts < params.VOLTAGE_ON) {
    // charging system off, check if relay is on and start the timer if it hasn't already been started.
    if(relayState && !delaySwitch) {  
      time = now;
      delaySwitch = true;
    }
  } else if(volts < params.VOLTAGE_LOW) {
    // low voltage, turn the relay off now
    relayOff();
  }

  if(initialState && (now - pd_time > (1000 * (unsigned long)params.PD_WAIT)))
  {
    initialState = false;
    if(!relayState)
      powerDown = true;
  }

  if(!powerDown || initialState) // wait for 500ms if we aren't powering down
  {
#ifdef CLI
    if(Serial.available()) {
      // CLI is active, reset pd_time to now to inhibit powerdown by another PD_WAIT
      pd_time = now;
    }
    cmdPoll();
#endif
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


#if defined(DEBUG) || defined(CLI)
void printStatus(unsigned long now, float v)
{
  unsigned long n = micros();
  char buf1[8];
  char buf2[128];
  dtostrf(v, 4, 2, buf1);
  snprintf(buf2, sizeof(buf2), "%lu %lu relay: %d, delay: %d, i: %d, pD: %d, V: %s V(m): ",
              now, n-now, relayState, delaySwitch, initialState, powerDown, buf1);
  Serial.print(buf2);
  Serial.println(voltmedian.getMedian());  
}
#endif

void loadParams()
{
  // load params from eeprom
  if(EEPROM.read(EEPROM_START) == params.CHECK) { // check for our CHECK byte is the same as the default
    for (unsigned int i=0; i<sizeof(params); i++)
      *((char*)&params+i) = EEPROM.read(EEPROM_START + i);
  } else {
    // params are uninitialized, write the defaults to memory
    saveParams(); 
  }
}

void saveParams()
{
   for(unsigned int i=0; i<sizeof(params); i++)
     EEPROM.write(EEPROM_START+i,*((char*)&params+i));
}

#ifdef CLI
void help(int argc, char **args)
{
  Serial.println("Command list:");
  Serial.println("  help                - this message");
  Serial.println("  status              - brief status");
  Serial.println("  list                - list parameters");
  Serial.println("  set <param> <value> - set parameter for this session");
  Serial.println("  save                - save parameters to eeprom");
  Serial.println("  sleep               - immediately enter powerdown mode (will end USB shell)");
}

void bstatus(int argc, char **args)
{
  printStatus(micros(),volts);
}

void list(int argc, char **args)
{
  char buf[90];
  char v1[8];
  char v2[8];
  char v3[8];
  dtostrf(params.VOLTAGE_LOW, 4, 2, v1);
  dtostrf(params.VOLTAGE_ON, 4, 2, v2);
  dtostrf(params.VDROP_PCT, 4, 3, v3);
  snprintf(buf, sizeof(buf), "OFF_WAIT: %d\n\rPD_WAIT: %d\n\rVOLTAGE_LOW: %s\n\rVOLTAGE_ON: %s\n\rVDROP_PCT: %s\n\r",
            params.OFF_WAIT, params.PD_WAIT, v1, v2, v3);
  Serial.print(buf);
}

void set(int argc, char **args)
{
  if(argc == 3) {
    char *p = args[1];
    if(!strcmp(p, "OFF_WAIT")) {
      params.OFF_WAIT = atoi(args[2]);
    } else if(!strcmp(p,"PD_WAIT")) {
      params.PD_WAIT = atoi(args[2]);
    } else if(!strcmp(p,"VOLTAGE_LOW")) {
      params.VOLTAGE_LOW = atof(args[2]);
    } else if(!strcmp(p,"VOLTAGE_ON")) {
      params.VOLTAGE_ON = atof(args[2]);
    } else if(!strcmp(p,"VDROP_PCT")) {
      params.VDROP_PCT = atof(args[2]);
    } else {
      Serial.println("Error: unknown parameter. Try list. Parameters are case sensitive.");  
      return;
    }
    Serial.println("Success");
  } else {
    Serial.println("Error: syntax: set <param> <value>");
  }  
}

void save(int argc, char **args)
{
  saveParams(); 
  Serial.println("Success");
}

void sleep(int argc, char **args)
{
  Serial.println("Setting sleep mode, goodbye!");
  powerDown = true;
  initialState = false;
}
#endif

