/*
    Adopted from Mads Aasvik "Creating a Command Line Interface in Arduino’s Serial Monitor"
    https://www.norwegiancreations.com/2018/02/creating-a-command-line-interface-in-arduinos-serial-monitor/

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

#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>
#include "ESPIoTSensor.h"
#include "serialCLI.h"

extern struct params myParams;

bool error_flag = false;

char line[LINE_BUF_SIZE];
char args[MAX_NUM_ARGS][ARG_BUF_SIZE];

int cmd_set();
int cmd_save();
void help_set();
void help_save();




//List of commands with function pointers
const strCmd commands[]={
	{"help",help_help,cmd_help},
	{"save",help_save,cmd_save},
	{"set",help_set,cmd_set}
};


//List of SET parameters command names
const strSetPara set_para[] = {
	{"wifi_ssid", { .pstr = myParams.ssid}, pt_string},
	{"wifi_passwd", { .pstr = myParams.password}, pt_string},
	{"wifiTimeout", { .pulong = &myParams.wifiTimeout}, pt_ulong},
	{"dbase_host", { .pstr = myParams.influxDBHost}, pt_string},
	{"dbase_port", {.puint16 = &myParams.influxDBPort}, pt_uint16},
	{"sensor_name", { .pstr = myParams.database}, pt_string},
	{"dbase_user", { .pstr = myParams.dbUser}, pt_string},
	{"sensor_name", { .pstr = myParams.sensName}, pt_string},
	{"location_tag", { .pstr = myParams.dbLocationTag}, pt_string},
	{"dbase_passwd", { .pstr = myParams.dbPasswd}, pt_string},
	{"sleep_time", {.pulong = &myParams.sleepTime}, pt_ulong},
	{"enable_sleep", {.pbool = &myParams.sleep}, pt_bool}
};

int num_commands = sizeof(commands) / sizeof(strCmd);
int num_set_para = sizeof(set_para) / sizeof(strSetPara);


void cli_init(){
	Serial.println("Welcome to this simple Arduino command line interface (CLI).");
	Serial.println("Type \"help\" to see a list of commands.");
	Serial.print("> ");
}

bool read_line(){
	static String line_string;
	char c=' ';

	while(Serial.available()){
		c=Serial.read();
		if('\n'==c)
		break;
		line_string+=c;
	}
	if('\n'==c){
		if(line_string.length() < LINE_BUF_SIZE){
			line_string.toCharArray(line, LINE_BUF_SIZE);
			Serial.println(line_string);
			line_string="";
			return true;
		}
		else{
			Serial.println("Input string too long.");
			error_flag = true;
		}
		line_string="";
	}
	return false;
}

void parse_line(){
	char *argument;
	int counter = 0;
	const char* ws=" \n\r\t";

	argument = strtok(line, ws);

	while((argument != NULL)){
		if(counter < MAX_NUM_ARGS){
			if(strlen(argument) < ARG_BUF_SIZE){
				strcpy(args[counter],argument);
				argument = strtok(NULL, ws);
				counter++;
			}
			else{
				Serial.println("Input string too long.");
				error_flag = true;
				break;
			}
		}
		else{
			break;
		}
	}
}

int execute(){  
	for(int i=0; i<num_commands; i++){
		if(strcmp(args[0], commands[i].cmd) == 0){
			return(*commands[i].exec_ptr)();
		}
	}

	Serial.println("Invalid command. Type \"help\" for more.");
	return 0;
}


void my_cli(){
	
	if(read_line()){
		if(!error_flag){
			parse_line();
		}
		if(!error_flag){
			execute();
		}

		memset(line, 0, LINE_BUF_SIZE);
		memset(args, 0, sizeof(args[0][0]) * MAX_NUM_ARGS * ARG_BUF_SIZE);

		error_flag = false;
		Serial.print("> ");
	}
}

void help_help(){
	Serial.println("The following commands are available:");

	for(int i=0; i<num_commands; i++){
		Serial.print("  ");
		Serial.println(commands[i].cmd);
	}
	Serial.println("");
	Serial.println("You can for instance type \"help [command]\" for more info on the set command.");
}

void help_set(){
	Serial.println("Set various parameters: ");
	for(int i=0;i<num_set_para;i++){
		Serial.println(String(String("set ") + set_para[i].parameter+ " [value]"));
	}
}

void help_save(){
	Serial.println("Save all parameters to EEPROM");
}


int cmd_help(){
	if(args[1] == NULL){
		help_help();
	} else{
		for(int i=0; i<num_commands; i++){
			if(strcmp(args[1], commands[i].cmd) == 0){
				(*commands[i].help_ptr)();
				return 0;
			}
		}
		help_help();
	}
}


int cmd_set(){
	if(0!=strlen(args[1])){
		Serial.println(args[1]);
		for(int i=0; i<num_set_para; i++){
			Serial.print(i);
			Serial.print(": ");
			Serial.print(set_para[i].parameter);
			Serial.print(": ");
			if(strcmp(args[1], set_para[i].parameter) == 0){
				Serial.println(args[2]);
				if(args[2] != NULL){
					switch (set_para[i].type){
					case pt_int:
						*set_para[i].pint=atoi(args[2]);
						break;
					case pt_string:
						strcpy(set_para[i].pstr,args[2]);
						break;
					case pt_uint16:
						*set_para[i].puint16=atoi(args[2]);
						break;
					case pt_ulong:
						*set_para[i].pulong=atol(args[2]);
						break;
					case pt_float:
						*set_para[i].pfloat=atof(args[2]);
						break;
					case pt_bool:
						if(atoi(args[2])>0)
							*set_para[i].pbool=true;
						else
							*set_para[i].pbool=false;
						break;
					}
					return 0;
				}
			}
		}
		Serial.println(strlen(args[1]));
		help_set();
	} else {
		for(int i=0; i<num_set_para; i++){
			Serial.print( set_para[i].parameter);
			Serial.print( " = ");
			switch (set_para[i].type){
			case pt_int:
				Serial.println(*set_para[i].pint);
				break;
			case pt_string:
				Serial.println(set_para[i].pstr);
				break;
			case pt_uint16:
				Serial.println(*set_para[i].puint16);
				break;
			case pt_ulong:
				Serial.println(*set_para[i].pulong);
				break;
			case pt_float:
				Serial.println(*set_para[i].pfloat);
				break;
			case pt_bool:
				Serial.println(*set_para[i].pbool);
				break;
			}
		}		
	}
}

int cmd_save(){
	EEPROM.begin(sizeof(params));
	EEPROM.put(0,myParams);
	EEPROM.end();
	Serial.println("All parameters saved to EEPROM");
	Serial.println("Restarting ...");
	ESP.reset();
}
