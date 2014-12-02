<pre>
BatteryTimer
============

Arduino Micro-based programmable relay switch with voltage sensing.

Includes serial CLI support for changing initial parameters to user-defined settings.

Theory of operation:
  At initial power up, default settings are saved to EEPROM. Subsequent reboots will read these values. Relay is turned off unless
  voltage is above VOLTAGE_ON. Initially, the firmware remains in a fully on initial state until PD_WAIT seconds elapse. Data received 
  via the CLI resets this timer. After PD_WAIT seconds, the board goes into full powerdown and wakes up every second to check sense the 
  input voltage. If the voltage is sensed to be greater than or equal to VOLTAGE_ON, the relay is engaged and powerdown mode is disabled.
  Once the voltage drops below VOLTAGE_ON, the relay will remain engaged until OFF_WAIT seconds elapse. Should the voltage drop below
  VOLTAGE_LOW at any time, the relay will immediately be disengaged and powerdown mode will be activated.
  
Parameters:
  VOLTAGE_ON  : voltage (float) at which the relay engages
  VOLTAGE_LOW : voltage (float) at which the relay is immediately disengaged and powerdown mode activated 
  OFF_WAIT    : time in seconds for the relay to remain engaged after voltage drops below VOLTAGE_ON
  PD_WAIT     : time in seconds to wait initially before entering powerdown mode. This allows the user to connect via CLI before USB is disabled.
  
Power consumption:
  Using boards acquired from China via Ebay (with power LED removed), this project has been measured at 240 microamps current draw while in powerdown.
  Full on current draw with relay engaged is ~70mA, full on current draw with relay open is ~32mA. 
</pre>