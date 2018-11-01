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
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <InfluxDb.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include "ESPIoTSensor.h"
#include "serialCLI.h"
#include "webserver.h"

ADC_MODE(ADC_VCC);

struct settings currSettings;
String esp_chipid;
Adafruit_BME280 bme; // I2C
float h, t, p;
float vBatt  = 0;
bool enable_sleep;


ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Influxdb* influxdb;
extern ESP8266WebServer server;
extern long last_page_load;

void save_settings(){
	uint8_t * ptr;
	unsigned int sum;
	
	ptr=(uint8_t *)&currSettings;
	sum=0;
	for(int i=0;i<sizeof(settings);i++){
		sum+=ptr[i];
	}
	EEPROM.begin(sizeof(settings)+sizeof(sum));
	EEPROM.put(0,currSettings);
	EEPROM.put(sizeof(settings),sum);
	EEPROM.end();
}

void load_settings(){
	uint8_t * ptr;
	unsigned int sum;
	unsigned int EEPROM_sum;
	struct settings savedSettings;
	
	Serial.println("Loading settins from EEPROM ...");
	EEPROM.begin(sizeof(settings)+sizeof(sum));
	EEPROM.get(0,savedSettings);
	EEPROM.get(sizeof(settings),EEPROM_sum);
	ptr=(uint8_t *)	&savedSettings;
	sum=0;
	for(int i=0;i<sizeof(settings);i++){
		sum+=ptr[i];
	}
	if(sum==EEPROM_sum){
		currSettings=savedSettings;
	} else {
		Serial.println("");
		Serial.println(F("EEPROM checksum does not match!"));
		Serial.println(F("Using default settings."));
		//		currSettings=savedSettings;
	}
	EEPROM.end();
}

void trySleep(){
	if(enable_sleep){
		Serial.println("");
		Serial.println("Sleeping ...");
		ESP.deepSleep(currSettings.sleepTime*1000000);
	}
}

void ap_config(){
	char ap_ssid[33] = "";
	char ap_pwd[65] = "";
	DNSServer dnsServer;
	IPAddress apIP(192, 168, 4, 1);
	IPAddress netMsk(255, 255, 255, 0);
	
	Serial.println("");
	(String(AP_SSID) + "-" + esp_chipid).toCharArray(ap_ssid,sizeof(ap_ssid));
	strcpy(ap_pwd, AP_PWD);

	Serial.println(String( F("Setting configuration access point ")) + ap_ssid);
	Serial.println(String( F("Access point password: '")) + ap_ssid + "'");
	Serial.println(String( F("Configure at: 'http://")) + apIP.toString() + "/'");
	
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(ap_ssid, ap_pwd);

	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);
	setup_webserver();

	while (((millis() - last_page_load) < TIME_FOR_WIFI_CONFIG)) {
		dnsServer.processNextRequest();
		server.handleClient();
	}

}

bool connect_WiFi(){
	// Connecting to WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(currSettings.ssid);
	
	//WiFi.begin(ssid, password);
	WiFiMulti.addAP(currSettings.ssid, currSettings.password);
	unsigned long start_time=millis();
	while (WiFiMulti.run() != WL_CONNECTED) {
		my_cli();
		delay(500);
		Serial.print(".");
		if(millis()-start_time > currSettings.wifiTimeout){
			Serial.println("");
			Serial.print("Timeout when connecting to ");
			Serial.println(currSettings.ssid);
			if(!enable_sleep){
				ap_config();
			}
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


// only runs once on boot
void setup() {
	Serial.begin(9600);
	delay(10);
	//	Wire.begin(D3, D4);
	Wire.begin(D1, D2);
	Wire.setClock(100000);
	
	load_settings();
	influxdb= new Influxdb(currSettings.influxDBHost, currSettings.influxDBPort);
	pinMode(D5, INPUT_PULLUP);
	
	if(digitalRead(D5)){
		enable_sleep=currSettings.sleep;
	}
	else {
		enable_sleep=false;
	}
	
	// Initializing serial port for debugging purposes

	esp_chipid = String(ESP.getChipId());

	Serial.println(F("BME280 init"));
	if (!bme.begin()) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
		trySleep();
		//		while (1);
	} else {
		bme.setSampling(
		Adafruit_BME280::MODE_FORCED,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::FILTER_OFF);
	}

	cli_init();
	if(!enable_sleep){
		Serial.println("");
		Serial.println(F("Configure mode ..."));
		connect_WiFi();
		setup_webserver();
		Serial.println("HTTP server started");
	}

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
	if(!enable_sleep){
		server.handleClient();
	}
	my_cli();
	if((millis()-last_millis>currSettings.sleepTime*1000) || first){
		last_millis=millis();
		if (WiFiMulti.run() != WL_CONNECTED){
			connect_WiFi();
		} 
		Serial.println("");
		Serial.println("Starting new measurement:");
		last_millis=millis();
		first=false;
		getBME280data();
		//		vBatt = (analogRead (A0)*5.66)/1024;
		vBatt = ESP.getVcc()/1000.;
		Serial.println();
		Serial.println("Humidity: " + String(h) + " %");
		Serial.println("Temperature: " + String(t) + " deg");
		Serial.println("Pressure: " + String(p) + " Pa");
		Serial.println("VBatt: " + String(vBatt) + " V");
		
		if (WiFiMulti.run() == WL_CONNECTED){
			timeClient.update();
			Serial.println("UTC Time: " + timeClient.getFormattedTime());
			Serial.println();
			influxdb->setDbAuth(currSettings.dbName, currSettings.dbUser, currSettings.dbPasswd);
			InfluxData m (currSettings.sensName);
			m.addTag("node", "esp8266-"+esp_chipid);
			m.addTag("location", currSettings.dbLocationTag);
			m.addValue("BME280_temperature", t);
			m.addValue("BME280_pressure", p);
			m.addValue("BME280_humidity", h);
			m.addValue("esp8266_vBatt", vBatt);
			influxdb->write(m);
		}
		Serial.print("> ");
		trySleep();
	}
} 