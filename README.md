<pre>
BatteryTimer
============

Arduino Micro-based programmable relay switch with voltage sensing.

Includes serial CLI support for changing initial parameters to user-defined settings.

Theory of operation:
  At initial power up, default settings are saved to EEPROM. Subsequent reboots will 
  read these values. Relay is turned off unless voltage is above VOLTAGE_ON. Initially,
  the firmware remains in a fully on initial state until PD_WAIT seconds elapse. Data 
  received via the CLI resets this timer. After PD_WAIT seconds, the board goes into 
  full powerdown and wakes up every second to sense the input voltage. If the 
  voltage is sensed to be greater than or equal to VOLTAGE_ON, the relay is engaged 
  and powerdown mode is disabled. Once the voltage drops below VOLTAGE_ON, the relay 
  will remain engaged until OFF_WAIT seconds elapse. Should the voltage drop below
  VOLTAGE_LOW at any time, the relay will immediately be disengaged and powerdown mode 
  will be activated.
  
Parameters:
  VOLTAGE_ON  : voltage (float) at which the relay engages. Default: 13.2
  VOLTAGE_LOW : voltage (float) at which the relay is immediately disengaged and 
                powerdown mode activated. Default: 10.8
  OFF_WAIT    : time in seconds for the relay to remain engaged after voltage drops 
                below VOLTAGE_ON. Default: 120 (2 minutes)
  PD_WAIT     : time in seconds to wait initially before entering powerdown mode. 
                This allows the user to connect via CLI before USB is disabled.
				Default: 45 seconds
  VDROP_DETECT: Flag to enable voltage drop detection to reset delay timer (1 or 0)
  VDROP_PCT   : Multiplied by median voltage and then compared to current voltage, 
                if voltage is less it will reset the timer (in-use detect). Default
				value is 0.99 (which is 1% drop or 0.115v at 11.5v)
  
Power consumption:
  Using Arduino Micro (atmega32u4) boards acquired from China via Ebay (with power LED 
  removed), this project has been measured at 240 microamps current draw while in 
  powerdown. Full on current draw with relay engaged is ~70mA, full on current draw with 
  relay open is ~32mA. 

Using the CLI:
  Connect the Arduino Micro board via USB and establish a serial connection at 9600bps 
  within PD_WAIT. Press enter to activate the CLI and reset the PD_WAIT timer. Type
  help for a list of commands and their use. Note that changes to parameters are not
  saved until the "save" command is issued.
  
Voltage drop detection:
  The current voltage is added to an array used to compute the running median voltage
  when the system is in OFF_WAIT. The median voltage is multiplied by VDROP_PCT and
  compared to the current voltage reading. If the current voltage reading is less 
  than VDROP_PCT*median_voltage, the off timer is reset and the controlled device will
  remain powered on for another OFF_WAIT seconds.
</pre>