/*
	Copyright (C) 2018 T. Mertelj
	
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Wire.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <InfluxDb.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include "ESPIoTSensor.h"
#include "serialCLI.h"

struct params myParams;
String esp_chipid;
Adafruit_BME280 bme; // I2C
float h, t, p;
float vBatt  = 0;

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Influxdb influxdb(myParams.INFLUXDB_HOST, myParams.INFLUXDB_PORT);



void trySleep(){
	if(myParams.sleep && digitalRead(D5)){
		Serial.println("");
		Serial.println("Sleeping ...");
		ESP.deepSleep(myParams.sleepTime*1e6);
	}
}

// only runs once on boot
void setup() {
	
	EEPROM.begin(sizeof(params));
	EEPROM.get(0,myParams);
	EEPROM.end();

	pinMode(D5, INPUT_PULLUP);
	
	// Initializing serial port for debugging purposes
	Serial.begin(9600);
	delay(10);
//	Wire.begin(D3, D4);
	Wire.begin(D1, D2);
	Wire.setClock(100000);

	esp_chipid = String(ESP.getChipId());

	Serial.println(F("BME280 init"));
	if (!bme.begin()) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
		trySleep();
		while (1);
	} else {
		bme.setSampling(
		Adafruit_BME280::MODE_FORCED,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::FILTER_OFF);
	}

	cli_init();

}

bool connect_WiFi(){
	// Connecting to WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(myParams.ssid);
	
	//WiFi.begin(ssid, password);
	WiFiMulti.addAP(myParams.ssid, myParams.password);
	unsigned long start_time=millis();
	while (WiFiMulti.run() != WL_CONNECTED) {
		my_cli();
		delay(500);
		Serial.print(".");
		if(millis()-start_time > myParams.wifi_timeout){
			Serial.println("");
			Serial.print("Timeout when connecting to ");
			Serial.println(myParams.ssid);
			return false;
		}
	}
	timeClient.begin();
	Serial.println("");
	Serial.println("WiFi connected");


	// Printing the ESP IP address
	Serial.println(WiFi.localIP());
	return true;
}

void getBME280data() {
	bme.takeForcedMeasurement();
	h = bme.readHumidity();
	t = bme.readTemperature();
	p = bme.readPressure();

}

// runs over and over again
void loop() {
	static unsigned long last_millis;
	static bool first=true;
	
	my_cli();
	if((millis()-last_millis>myParams.sleepTime*1000) || first){
		last_millis=millis();
		if (WiFiMulti.run() != WL_CONNECTED){
			connect_WiFi();
		} 
		Serial.println("");
		Serial.println("Starting new measurement:");
		last_millis=millis();
		first=false;
		getBME280data();
		vBatt = (analogRead (A0)*5.66)/1024;
		Serial.println();
		Serial.println("Humidity: " + String(h) + " %");
		Serial.println("Temperature: " + String(t) + " deg");
		Serial.println("Pressure: " + String(p) + " Pa");
		Serial.println("VBatt: " + String(vBatt) + " V");
		
		if (WiFiMulti.run() == WL_CONNECTED){
			timeClient.update();
			Serial.println("UTC Time: " + timeClient.getFormattedTime());	
			influxdb.setDbAuth(myParams.DATABASE, myParams.DB_USER, myParams.DB_PASSWORD);
			InfluxData m ("feinstaub");
			m.addTag("node", "esp8266-"+esp_chipid);
			m.addValue("BME280_temperature", t);
			m.addValue("BME280_pressure", p);
			m.addValue("BME280_humidity", h);
			m.addValue("esp8266_vBatt", vBatt);
			influxdb.write(m);
		}
		Serial.print("> ");
		trySleep();
	}
} 