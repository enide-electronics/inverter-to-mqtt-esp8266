# Building it

This project was originally written in the Arduino "IDE" version 1.8.16 but I got tired of waiting forever for the builds to complete so I decided to try the Platform IO (which runs on the Visual Studio Code) IDE.

To build it, open the project in Platform IO and select one of the build tasks.

This project builds on top of the work of many different persons. The dependencies, automatically managed by Platform IO are:
- esp8266 core versions 2.6.3 & 2.7.4 (both work fine and I prefer the latter)
- pubsubclient version 2.8.0
- modbusmaster version 2.0.1
- wifimanager version 2.0.5-beta
- arduinojson version 6.19.2
- stringsplitter 1.0.0

:warning: If you plan on running the ESP8266 board connected to your computer to debug changes you made to the code, **make sure to not power the board from the inverter serial pin 9** otherwise you'll risk frying the ESP module, your computer or the inverter. **Remove the jumper to power the board from the USB cable only.**

Remember the Growatt and Soyosource inverters are non-isolated inverters.
