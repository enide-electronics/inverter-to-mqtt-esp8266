# Tele topics
These topics are published every minute. The `<name>` part corresponds to value in the `MQTT base topic`.

| Topic                      | Units | Format | Description                                                           |
|----------------------------|-------|--------|-----------------------------------------------------------------------|
| `<name>/tele/IP`           | -     | text   | Board IP address                                                      |
| `<name>/tele/Uptime`       | -     | text   | Uptime                                                                |
| `<name>/tele/ClientID`     | -     | text   | MQTT client ID                                                        |
| `<name>/tele/RSSI`         | -     | int    | ESP8266 WiFi RSSI value in dBm, negative number                       |
|----------------------------|-------|--------|-----------------------------------------------------------------------|

# Growatt MQTT Topics
Please note that the "growatt" prefix in all topics shown below is the one selected for my Growatt inverter. It is configurable via the web interface if you want to change it. [See here](README.md).

## Energy Data
Energy data is polled periodically from the inverter Input Registers, every N seconds defined via the web interface. These topics are available for all supported Growatt inverter types.

| Topic                       | Units | Format | Description                                                           |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/online`            | -     | bool   | MQTT connection status (Last will and testament)                      |
| `growatt/status`            | -     | int    | Inverter numeric status                                               |
| `growatt/Priority`          | -     | text   | Inverter working priority (Load, Bat, Grid)                           |
| `growatt/DerateMode`        | -     | int    | Derating due to overtemp, overvoltage, unstable frequency, etc.       |
| `growatt/Derate`            | -     | text   | Derating textual cause                                                |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/Vpv1`              | V     | float  | Voltage of solar string 1                                             |
| `growatt/Ppv1`              | W     | float  | Power of solar string 1                                               |
| `growatt/Ipv1`              | A     | float  | Current of solar string 1                                             |
| `growatt/Vpv2`              | V     | float  | Voltage of solar string 2                                             |
| `growatt/Ppv2`              | W     | float  | Power of solar string 2                                               |
| `growatt/Ipv2`              | A     | float  | Current of solar string 2                                             |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/Vac1`              | V     | float  | Grid Phase 1 voltage                                                  | 
| `growatt/Iac1`              | A     | float  | Grid Phase 1 current                                                  |
| `growatt/Pac1`              | VA    | float  | Grid Phase 1 apparent power                                           |
| `growatt/Vac2`              | V     | float  | Grid Phase 2 voltage (disabled by default, TL inverters only)         | 
| `growatt/Iac2`              | A     | float  | Grid Phase 2 current (disabled by default, TL inverters only)         |
| `growatt/Pac2`              | VA    | float  | Grid Phase 2 apparent power (disabled by default, TL inverters only)  |
| `growatt/Vac3`              | V     | float  | Grid Phase 3 voltage (disabled by default, TL inverters only)         | 
| `growatt/Iac3`              | A     | float  | Grid Phase 3 current (disabled by default, TL inverters only)         |
| `growatt/Pac3`              | VA    | float  | Grid Phase 3 apparent power (disabled by default, TL inverters only)  |
| `growatt/Pac`               | W     | float  | Grid Tie inverter output ACTIVE power                                 |
| `growatt/Fac`               | Hz    | float  | Grid Tie inverter output/grid frequency                               |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/Etoday`            | kWh   | float  | Total energy produced today                                           |
| `growatt/Etotal`            | kWh   | float  | Total energy produced                                                 |
| `growatt/Ttotal`            | Hours | float  | Total time inverter running                                           |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/Temp1`             | ºC    | float  | Temperature of inverter                                               |
| `growatt/Temp2`             | ºC    | float  | Temperature inside IPM of inverter                                    |
| `growatt/Temp3`             | ºC    | float  | Temperature of boost module                                           |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/Battery`           | -     | text   | Type of battery (Lithium, LeadAcid or Unknown)                        |
| `growatt/Pdischarge`        | W     | float  | Battery discharge power                                               |
| `growatt/Pcharge`           | W     | float  | Battery charge power                                                  |
| `growatt/Vbat`              | V     | float  | Battery voltage                                                       |
| `growatt/SOC`               | %     | int    | Battery charge percentage (state of charge)                           |
|-----------------------------|-------|--------|-----------------------------------------------------------------------|
| `growatt/EpsFac`            | Hz    | float  | EPS output frequency                                                  |
| `growatt/EpsVac1`           | V     | float  | EPS Phase 1 voltage                                                   | 
| `growatt/EpsIac1`           | A     | float  | EPSPhase 1 current                                                    |
| `growatt/EpsPac1`           | W     | float  | EPS Phase 1 total power                                               |
| `growatt/EpsVac2`           | V     | float  | EPS Phase 2 voltage (disabled by default, for TL inverters only)      | 
| `growatt/EpsIac2`           | A     | float  | EPSPhase 2 current (disabled by default, for TL inverters only)       |
| `growatt/EpsPac2`           | W     | float  | EPS Phase 2 total power (disabled by default, for TL inverters only)  |
| `growatt/EpsVac3`           | V     | float  | EPS Phase 3 voltage (disabled by default, for TL inverters only)      | 
| `growatt/EpsIac3`           | A     | float  | EPSPhase 3 current (disabled by default, for TL inverters only)       |
| `growatt/EpsPac3`           | W     | float  | EPS Phase 3 total power (disabled by default, for TL inverters only)  |
| `growatt/EpsLoadPercent`    | %     | float  | EPS Load in percentage 100% at 3kW                                    |
| `growatt/EpsPF`             | -     | float  | EPS power factor                                                      |



## Settings
Inverter settings are kept in Holding Registers. These are **only available for Growatt SPH inverters**.
These can be modified by publishing messages with the correct values to the following MQTT topics:

| Topic                                 | Value                                            | Parameter                                | Observations                                                        | 
|---------------------------------------|--------------------------------------------------|------------------------------------------|---------------------------------------------------------------------|
| `growatt/settings/led`                | `0` OR `1` OR `2`                                | Default ESP8266 LED behaviour            | LED always OFF (0), always ON (1) or blinking when polling data (2) |
| `growatt/settings/priority`           | `load` OR `bat` OR `grid`                        | Priority setting from the inverter menu  | `grid` is being tested and may require more work to work correctly  |
|  (same)                               | `status`                                         | Special priority                         | `status` returns all priority settings in JSON format (see below)   |
|---------------------------------------|--------------------------------------------------|------------------------------------------|---------------------------------------------------------------------|
| `growatt/settings/priority/bat/ac`    | `0` OR `1` OR `off` OR `on` OR `false` OR `true` | AC Charger                               | Enables or disables the AC Charger in Battery First                 |
| `growatt/settings/priority/bat/pr`    | `1` ... `100`                                    | Charge Power Rating                      | Battery First charging power rating                                 |
| `growatt/settings/priority/bat/ssoc`  | `13` ... `100`                                   | Stop State Of Charge                     | Battery First SSOC                                                  |
| `growatt/settings/priority/bat/t1`    | `00:00 23:59`                                    | Battery First Time                       | Battery First Time Interval 1 that can be set from the panel        |
|---------------------------------------|--------------------------------------------------|------------------------------------------|---------------------------------------------------------------------|
| `growatt/settings/priority/grid/pr`   | `1` ... `100`                                    | Discharge Power Rating                   | Grid First discharge power rating                                   |
| `growatt/settings/priority/grid/ssoc` | `13` ... `100`                                   | Stop State Of Charge                     | Grid First SSOC                                                     |
| `growatt/settings/priority/grid/t1`   | `00:00 23:59`                                    | Grid First Time                          | Grid First Time Interval 1 that can be set from the panel           |

### Reading the inverter priority settings
When publishing `status` to `growatt/settings/priority` the inverter replies with a JSON representation of all params you see inside the inverter priority menu.

Here's an example of the inverter currently working in Battery First:
```json
{
    "grid": {
        "pr": 100,
        "ssoc": 5,
        "t1": "00:00 00:00",
        "t1_enable": "off",
        "t2": "00:00 00:00",
        "t2_enable": "off",
        "t3": "00:00 00:00",
        "t3_enable": "off"
    },
    "bat": {
        "pr": 40,
        "ssoc": 57,
        "ac": "off",
        "t1": "00:00 23:59",
        "t1_enable": "on",
        "t2": "00:00 00:00",
        "t2_enable": "off",
        "t3": "00:00 00:00",
        "t3_enable": "off"
    },
    "load": {
        "t1": "00:00 00:00",
        "t1_enable": "off",
        "t2": "00:00 00:00",
        "t2_enable": "off",
        "t3": "00:00 00:00",
        "t3_enable": "off"
    }
}
```

## Settings currently not enabled in the code

:warning:  These topics **are supported** by the code but are disabled in the released binaries for safety.

:warning:  You can recompile the code to enable them if you want.

:warning:  Remember, if you set any of these values, they will not be visible on the inverter screen nor they can be modified through the panel.

:warning:  You will only be able to change them or disable them via their MQTT topics.

| Topic                               | Value         | Parameter            | Observations                  | 
|-------------------------------------|---------------|----------------------|-------------------------------|
| `growatt/settings/priority/bat/t2`  | `00:00 23:59` | Battery First Time 2 | Battery First Time Interval 2 |
| `growatt/settings/priority/bat/t3`  | `00:00 23:59` | Battery First Time 3 | Battery First Time Interval 3 |
|-------------------------------------|---------------|----------------------|-------------------------------|
| `growatt/settings/priority/grid/t2` | `00:00 23:59` | Grid First Time 2    | Grid First Time Interval 2    |
| `growatt/settings/priority/grid/t3` | `00:00 23:59` | Grid First Time 3    | Grid First Time Interval 3    |

## Polling multiple Growatt inverters on the same RS485
If you have more than one Growatt inverter, **of the same model**, and if they are all connected them to the same RS485 bus, you can poll their data (and do remote configuration if the code supports it) using a single ESP8266 board, also connected to the same RS485 bus.

All inverters need to be configured to have a different MODBUS address to avoid communication collisions. You may have to use the inverter panel or Growatt ShineBus software to set their modbus addresses.

### Configuration changes
To enable multi-inverter mode **more than one** modbus address needs to be set in the configuration.

You set all the inverters modbus addresses in the `WebUI -> Setup -> Inverter modbus address` field as a **comma-separated list** of numbers, without any spaces. Example: `1,42,6,3` assuming you have 4 inverters and their modbus addresses are 1, 3, 6 and 42.


### Topic name changes
All growatt topics shown in the sections above will _change slightly_ to reflect the inverter the belong to.

So, if we pick the `growatt/Vpv1` topic as an example, each inverter on the bus will publish their PV1 voltages as `growatt/1/Vpv1`, `growatt/42/Vpv1`, `growatt/6/Vpv1` and `growatt/3/Vpv1`, if we have the inverters configured in the previous section.

Their statuses (`growatt/status` topic) will be published to `growatt/1/status`, `growatt/42/status`, `growatt/6/status` and `growatt/3/status`.

In the end you need to add modbus address between the `name` and the `subtopic` for all topics. Also for the topic names used for remote configuration: `growatt/settings/priority` -> `growatt/3/settings/priority`, etc.

Basically, the pattern for the topic names in multi-inverter mode is `<inverterName>/<modbusId>/<subtopic>` while for single inverter mode is `<inverterName>/<subtopic>`

## Modbus and inverter registers
See [REGISTERS.md](REGISTERS.md) for more details.

# Soyosource GTN inverter Topics
Please note that the "gtn1200w" prefix in all topics shown below is the one selected for my Soyosource GTN 1200W inverter. It is configurable via the web interface if you want to change it. [See here](README.md).


## Energy Data
Energy data is polled periodically from the messages sent by the CPU to the display, every N seconds defined via the web interface.

| Topic                        | Units | Format | Description                                                           |
|------------------------------|-------|--------|-----------------------------------------------------------------------|
| `gtn1200w/online`            | -     | bool   | MQTT connection status (Last will and testament)                      |
|------------------------------|-------|--------|-----------------------------------------------------------------------|
| `gtn1200w/Error`             | -     | int    | Numeric error value                                                   |
| `gtn1200w/ErrorBitmask`      | -     | float  | Numeric error bitmask (no use)                                        |
| `gtn1200w/Fac`               | Hz    | float  | Grid frequency in Hz                                                  |
| `gtn1200w/Ibat`              | Amps  | float  | DC input current (PV or Battery)                                      |
| `gtn1200w/MeterConnected`    | -     | bool   | `yes` if meter connected, `no` otherwise                              |
| `gtn1200w/Mode`              | -     | int    | Inverter working mode                                                 |
| `gtn1200w/ModeString`        | -     | text   | Inverter working mode as text                                         |
| `gtn1200w/OperationStatus`   | -     | text   | Operation status (Normal, Standby)                                    |
| `gtn1200w/OperationStatusId` | -     | int    | Numeric Operation status                                              |
| `gtn1200w/Pac`               | Watts | float  | AC power being injected                                               |
| `gtn1200w/PacMeter`          | Watts | float  | AC power requested (stuck at 257 no matter what)                      |
| `gtn1200w/Pbat`              | Watts | float  | DC power being generated                                              |
| `gtn1200w/Temp`              | ºC    | float  | Inverter temperature                                                  |
| `gtn1200w/Vac`               | Volts | float  | AC grid voltage                                                       |
| `gtn1200w/Vbat`              | Volts | float  | DC input voltage (PV or Battery)                                      |
|------------------------------|-------|--------|-----------------------------------------------------------------------|
| `gtn1200w/tele/IP`           | -     | text   | Board IP address                                                      |
| `gtn1200w/tele/Uptime`       | -     | text   | Uptime                                                                |
| `gtn1200w/tele/ClientID`     | -     | text   | MQTT client ID                                                        |
| `gtn1200w/tele/RSSI    `     | -     | int    | ESP8266 WiFi RSSI value, between 0 and 255                            |
|------------------------------|-------|--------|-----------------------------------------------------------------------|

## Limiter / Meter function
The limiter/meter function of the Soyosource can be used by connecting the ESP8266 via RS485 to the inverter (see the connections diagram [here](HARDWARE.md#connections-diagram)). The ESP8266 listens to the power value (in Watts), to sent to the inverter every 250ms, on the MQTT show below.

| Topic                        | Units | Format | Description                                                           |
|------------------------------|-------|--------|-----------------------------------------------------------------------|
| `gtn1200w/settings/power`    | Watts | int    | Limiter/Meter demand power (to use the inverter in `PV Limit` mode), min = 0, max = 1200   |
|------------------------------|-------|--------|-----------------------------------------------------------------------|

# Voltronic Axpert VM III
TBD
This is an experimental feature.

## Energy data
TBD
Since this is an experimental feature, the topics currenctly defined may change.