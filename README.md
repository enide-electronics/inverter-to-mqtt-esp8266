# Inverter to MQTT on ESP8266 (formerly growatt-sph-spa-esp8266)

This project's main goal is to read energy data from solar inverters and publish this information to a MQTT server of your choice. 

Currently this project supports several Growatt models (using the v1.24 register map), the Soyosource GTN and the Voltronic Axpert VM III. More inverters will be added in the future.

Since most Growatt inverters share the same register map, it was tested successfully on the **Growatt SPH 5000** **Growatt MIN 3000TL-XH** and should also work on other models like the **Growatt SPA**.

The Soyosource GTN 1000/1200 W with the display is supported and the remove limiter function is also implemented.

Support for Voltronic inverters is being slowly added but currently the code for the Axpert VM III is being tested.

It will run on many ESP8266 boards like the NodeMCU v3, Wemos D1 or even ESP-01 albeigth with limited functionality, because it doesn't require anything besides the ESP chip and a serial converter (RS232 or RS485).

The inverter data is published to the MQTT server you configure and can be consumed by almost any Home Automation solution.

Many ideas to implement this project came from other projects, most notably:
- [growatt-esp8266](https://github.com/jkairys/growatt-esp8266)
- [growatt-rs232-reader](https://github.com/lemval/growatt-rs232-reader)
- [soyosource-wifi-monitor](https://github.com/Stefino76/soyosource-wifi-monitor)
- [esphome-soyosource-gtn-virtual-meter](https://github.com/syssi/esphome-soyosource-gtn-virtual-meter/blob/main/components/soyosource_display/soyosource_display.cpp)
- [voltronic_ESP8266_MQTT](https://github.com/amishv/voltronic_ESP8266_MQTT/blob/main/src/communication.cpp)
- [docker-voltronic-homeassistant](https://github.com/ned-kelly/docker-voltronic-homeassistant/blob/master/sources/inverter-cli/inverter.cpp)
  

## Main features
- All configuration done via web interface (captive portal and web)
- Selectable inverter type
- Periodically polls data from the inverter and publishes it to the MQTT server via Wifi
- Polling period is configurable (in seconds)
- Some inverters are remote controllable via MQTT topics. 
  - Example for Growatt SPH:
     - **Priority**: load, battery, grid
     - **Time intervals** (eg 00:00 - 23:59) for battery and grid priorities
     - Enable/Disable the **AC Charger**
     - Set **StopStateOfCharge** for battery and grid priorities
    - Set **PowerRating** for battery and grid priorities
  - Soyosource
    - Output power
- Prebuilt binaries for NodeMCU/WemosD1 and Generic ESP-01 ESP8266 modules, ie no need to recompile the code
- No cloud, all energy data is under your control

## Downloading a pre-built binary
There are currently two pre-built binary files available:
- `generic`: for ESP-01 boards as it uses the hardware serial to communicate with the inverter
- `d1mini`: for NodeMCU/WemosD1 and larger boards, it uses a SoftwareSerial port on pins D5 and D6 to communicate with the inverter... the USB port is used for log messages

### Build instructions
See [BUILD.md](BUILD.md) for more details, if you really want to compile it on your own.

## Configuration
Everything is **configured via WiFiManager's Captive Portal / Web Portal** and stored in the SPIFFS file system, in JSON files.

When powering up the board for the first time, after uploading the firmware, you'll be presented with a WiFi network named `inverter-to-mqtt-esp8266` from which you can start the configuratin process. Connecting to this network will open the Wifi settings where you can choose the network this board should connect to and optionally the static IP address, if you want to assign a static IP. For the static IP to be saved you'll need to fill all 4 IPs (IP, GW, MASK and DNS).

The board will then reboot and connect to the Wifi you selected and you should be able to access it and configure the remaining settings, ie, inverter type, polling period, MQTT server, etc.
### The main screen
<img src='images/ss01-main.png' width='320px'>

### The WiFi settings screen
<img src='images/ss02-wifi.png' width='320px'>

##
So after that initial wifi setup is complete, you will be able to access the web interface on the IP address assigned to the board (static or dynamic), on the wifi network it's connected to.

### The main screen
<img src='images/ss01-main.png' width='320px'>

All parameters can be changed on the fly without the need for a restart, except for the `Device name` which requires a restart because it is used on DHCP requests and as the WiFi SoftAP name.

### The setup screen with the remaining settings
<img src='images/ss03-setup.png' width='320px'>

##
Currently there are 5 different inverter availables:
* Growatt SPH (Hybrid single phase)
* Growatt SPH-TL (Hybrid three phase)
* Growatt MIN-XH (Grid tied, single phase)
* Soyosource GTN (display version only with limiter)
* Voltronic Axpert VMIII (Off-grid, currently under testing)
* Test (A test inverter that publishes random values, for testing purposes)
* None (Default after a factory reset, will not publish anything to the MQTT server besides the telemetry)

### MQTT
The complete list of MQTT topics used by this project is available in the [topics](TOPICS.md) file.
If you use Home Assistant, you can grab the list of preconfigured sensor entities from [the home assistant](HOMEASSISTANT.md) file to help you get started.

## Hardware
This project runs on any generic ESP8266 board, as long as you provide the means to interface it to the inverter serial port, using a MAX3232, a MAX485 or similar chips.
Look in the [hardware](HARDWARE.md) file for more details on how to connect an ESP8266 board to the Growatt SPH inverter.

