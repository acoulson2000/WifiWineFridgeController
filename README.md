# WifiWineFridgeController

This project comprises two sketches for Wifi-Enabling my peltier-cooled wine refrigerator and for regulating the
temperature by managing the power supplied to the Fan/Peltier thermocouple assemblies.

WineFridgeWifi.ino is intended to run on a Heltec "Wifi Kit 8" ESP8266 + 96x32 OLED board, which acts as the
"front-end" controller. It could be adapted to other ESP8266 + OLED combinations relatively easily. This sketch
provides a web interface for displaying and setting the fridge temps (with the option to have button-controlled
setting, as well), displays the temp, and sends the setting to an Arduino/Monster Moto shield combo on the backend
periodically over serial2 (provided via the SoftwareSerial lib).

The Arduino back-end regulates power to the fans and peltier heat sinks by varying the duty cycle of PWM pins connected
to a Monster Moto shield. WineCoolerDriver.ino is intended for the Arduino. It handles sensing the current temperature, 
which it sends back to the front-end periodically over serial2, and varying the PWM duty cycle base on the differnce
between the current and target temperatures.

Various debugging and diagnostics are printed on SERIAL by both sketches.

Much more detail is available on my blog post at:
 https://austinlightguy.wordpress.com/2018/02/06/wine-fridge-temperature-controller-v2/
 
Info on the my original effort, which was just to replace the proprietary power supply that burned out,
can be found on an earlier post here: https://austinlightguy.wordpress.com/2016/04/26/wine-cooler-power-supply/
