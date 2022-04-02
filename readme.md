A simple ESP8266 based weather BME280 sensor/logger that logs into an influxDB database.

Connect BME280 sensor to GPIO4 and GPIO5:

BME280-SDA <--> ESP8266-GPIO4	 

BME280-SCL <--> ESP8266-GPIO5	 

BME280-GND <--> ESP8266-GND

BME280-VCC <--> ESP8266-VCC (3.3 V)

In order to run from battery the sensor deep sleeps most of the time. In order to decrease current consumption to minimum modules with included regulators and USB <--> RS232 are not recommended.

For waking from sleep connect also ESP8266-GPIO16 <--> ESP8266-RST pins.
If ESP8266-GPIO14 is grounded sleep is disabled disregarding the software configuration. 

The device must be configured before the use via a simple command line interface (CLI) connected to the serial port or built-in web server. 

Use Arduino or serial terminal to acces CLI. Typed commands must be terminated by '\n' (Ctrl-j).

Commands:

help <command>

set [parameter] [value] 	

save

(Compiles under Arduino 1.6.13)