/*
	Copyright (C) 2018 T. Mertelj
	
	Based on airRohr firmware Copyright (C) 2016  Code for Stuttgart a.o.
	
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


#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "ESPIoTSensor.h"

extern struct params myParams;
extern String esp_chipid;

String server_name;
ESP8266WebServer server(80);
const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_SVG[] PROGMEM = "image/svg+xml";
const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM = "image/png";


const char WEB_PAGE_HEADER[] PROGMEM = "<html>\
<head>\
<title>{t}</title>\
<meta name='viewport' content='width=device-width'>\
<style type='text/css'>\
body{font-family:Arial;margin:0}\
.content{margin:10px}\
.r{text-align:right}\
td{vertical-align:top;}\
a{text-decoration:none;padding:10px;background:#38b5ad;color:white;display:block;width:auto;border-radius:5px;}\
input[type='text']{width:100%;}\
input[type='password']{width:100%;}\
input[type='submit']{border-radius:5px;font-size:medium;padding:5px;}\
.submit_green{padding:9px !important;width:100%;border-style:none;background:#38b5ad;color:white;text-align:left;}\
</style>\
</head><body>\
<div style='min-height:120px;background-color:#38b5ad;margin-bottom:20px'>\
<h3 style='margin:0'>{tt}</h3>\
<small>ID: {id}<br/>MAC: {mac}<br/>{fwt}: {fw}</small></div><div class='content'><h4>{h} {n} {t}</h4>";

String line_from_value(const String& name, const String& value) {
	String s = F("<br>{n}: {v}");
	s.replace("{n}", name);
	s.replace("{v}", value);
	return s;
}

String form_checkbox(const String& name, const String& info, const bool checked) {
	String s = F("<label for='{n}'><input type='checkbox' name='{n}' value='1' id='{n}' {c}/><input type='hidden' name='{n}' value='0' /> {i}</label><br/>");
	if (checked) {
		s.replace("{c}", F(" checked='checked'"));
	} else {
		s.replace("{c}", "");
	};
	s.replace("{i}", info);
	s.replace("{n}", name);
	return s;
}


String form_input(const String& name, const String& info, const String& value, const int length) {
	String s = F("<tr><td>{i} </td><td style='width:90%;'><input type='text' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/></td></tr>");
	s.replace("{i}", info);
	s.replace("{n}", name);
	s.replace("{v}", value);
	s.replace("{l}", String(length));
	return s;
}

String form_password(const String& name, const String& info, const String& value, const int length) {
	String password = "";
	for (int i = 0; i < value.length(); i++) {
		password += "*";
	}
	String s = F("<tr><td>{i} </td><td style='width:90%;'><input type='password' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/></td></tr>");
	s.replace("{i}", info);
	s.replace("{n}", name);
	s.replace("{v}", password);
	s.replace("{l}", String(length));
	return s;
}

String form_submit(const String& value) {
	String s = F("<tr><td>&nbsp;</td><td><input type='submit' name='submit' value='{v}' /></td></tr>");
	s.replace("{v}", value);
	return s;
}


/*****************************************************************
/* html helper functions                                         *
/*****************************************************************/

String make_header(const String& title) {
	String s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{tt}", F("ESPIoTSensor"));
	s.replace("{h}", F(""));
	s.replace("{n}", F(""));
	s.replace("{t}", title);
	s.replace("{id}", esp_chipid);
	s.replace("{mac}", WiFi.macAddress());
	s.replace("{fwt}", F("ESPIoTSensor"));
	s.replace("{fw}", F("0.01"));
	return s;
}


void webserver_root() {
	bool save=false;
	String page_content = make_header(F("Configure"));
	String masked_pwd = "";
	int i = 0;
//	last_page_load = millis();

	if (server.method() == HTTP_GET) {
		page_content += F("<form method='POST' action='/' style='width:100%;'><b>");
		page_content += F("WiFi Settings");
		page_content += F("</b><br/>");
		page_content += F("<table>");
		page_content += form_input("wlanssid", F("WLAN"), myParams.ssid, 64);
		page_content += form_password("wlanpwd", F("Password"), myParams.password, 64);
		page_content += form_input("wifiTimeout", F("WLAN Timeout"), String(myParams.wifiTimeout/1000), 64);
		page_content += F("</table><br/><hr/><b>");
		page_content += F("<b>InfluxDB Settings");
		page_content += F("</b><br/>");
		page_content += F("<table>");
		page_content += form_input("dbase_host", F("dbase_host"), myParams.influxDBHost, 50);
		page_content += form_input("dbase_port", F("dbase_port"), String(myParams.influxDBPort), 30);
		page_content += form_input("dbase_user", F("dbase_user"), myParams.dbUser, 50);
		page_content += form_password("dbase_passwd", F("dbase_passwd"), myParams.dbPasswd, 50);
		page_content += form_input("sensor_name", F("sensor_name"), myParams.sensName, 50);
		page_content += form_input("location_tag", F("location_tag"), myParams.dbLocationTag, 50);
		page_content += F("</table><br/><hr/><b>");
		page_content += F("<b>Sleep Settings");
		page_content += F("</b><br/>");
		page_content += F("<table>");
		page_content += form_checkbox("enable_sleep", F("enable_sleep"), myParams.sleep);
		page_content += form_input("sleep_time", F("sleep_time"), String(myParams.sleepTime), 50);
		page_content += F("</table><br/><hr/><b>");
		page_content += form_submit(F("Save"));


	} else {

#define readCharParam(param,dest) if (server.hasArg(#param)){ server.arg(#param).toCharArray(dest, sizeof(dest)); }
#define readBoolParam(param,dest) if (server.hasArg(#param)){ dest = server.arg(#param) == "1"; }
#define readIntParam(param, dest)  if (server.hasArg(#param)){ int val = server.arg(#param).toInt(); if (val > 0){ dest = val; }}
#define readTimeParam(param, dest)  if (server.hasArg(#param)){ int val = server.arg(#param).toInt(); if (val > 0){ dest = val*1000; }}
#define readPasswdParam(param,dest) if (server.hasArg(#param)){ i = 0; masked_pwd = ""; for (i=0;i<server.arg(#param).length();i++) masked_pwd += "*"; if (masked_pwd != server.arg(#param) || server.arg(#param) == "") { server.arg(#param).toCharArray(dest, sizeof(dest)); } }

		if (server.hasArg("wlanssid") && server.arg("wlanssid") != "") {
			readCharParam(myParams.ssid, myParams.ssid);
			readPasswdParam(myParams.password, myParams.password);
		}
		readTimeParam(wifiTimeout, myParams.wifiTimeout);
		readCharParam(dbase_host, myParams.influxDBHost);
		readIntParam(dbase_port, myParams.influxDBPort);
		readCharParam(dbase_user,myParams.dbUser);
		readPasswdParam(dbase_passwd, myParams.dbPasswd);
		readCharParam(sensor_name,myParams.sensName);
		readCharParam(location_tag,myParams.dbLocationTag);
		readIntParam(sleep_time, myParams.sleepTime);
		readBoolParam(enable_sleep,myParams.sleep);
#undef readCharParam
#undef readBoolParam
#undef readIntParam
		page_content += line_from_value(F("WLAN"),myParams.ssid);
		page_content += line_from_value(F("wifiTimeout"),String(myParams.wifiTimeout/1000));
		page_content += line_from_value(F("dbase_host"), myParams.influxDBHost);
		page_content += line_from_value(F("dbase_port"), String(myParams.influxDBPort));
		page_content += line_from_value(F("dbase_user"), myParams.dbUser);
		page_content += line_from_value(F("sensor_name"), myParams.sensName);
		page_content += line_from_value(F("location_tag"), myParams.dbLocationTag);
		page_content += line_from_value(F("enable_sleep"), String(myParams.sleep));
		page_content += line_from_value(F("sleep_time"), String(myParams.sleepTime));
		page_content += F("<br/>");
		page_content += F("<br/>");
		page_content += F("All parameters saved to EEPROM. Restarting ...");
		page_content += "<br/><br/><a href='/' style='display:inline;'>Back to Config Page</a><br/><br/><br/>";
		save=true;
	}
	page_content += "</div></body></html>\r\n";
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
	if(save){
		EEPROM.begin(sizeof(params));
		EEPROM.put(0,myParams);
		EEPROM.end();
		delay(500);
		ESP.reset();
	}
}

/*****************************************************************
/* Webserver page not found                                      *
/*****************************************************************/
void webserver_not_found() {
	server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Page not found."));
}

/*****************************************************************
/* Webserver setup                                               *
/*****************************************************************/
void setup_webserver() {
	server_name  = F("ESPIoTSensor-");
	server_name += esp_chipid;
	server.on("/", webserver_root);
	server.onNotFound(webserver_not_found);

	server.begin();
}
