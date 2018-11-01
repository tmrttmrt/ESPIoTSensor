#ifndef ESPIoTSensor_h
#define ESPIoTSensor_h

#define MAX_PARA_LEN 64

struct params {
	
	char ssid[MAX_PARA_LEN] = "some-ssid";
	char password[MAX_PARA_LEN] = "some-passwd";
	char influxDBHost[MAX_PARA_LEN] = "some-host";
	uint16_t influxDBPort = 8086;
	char database[MAX_PARA_LEN] = "some-database";
	char dbUser[MAX_PARA_LEN] = "some-database-user";
	char dbPasswd[MAX_PARA_LEN] = "some-database-password";
	char sensName[MAX_PARA_LEN] = "sensor-name";
	char dbLocationTag[MAX_PARA_LEN] = "some-location";
	unsigned long sleepTime=30000000;
	unsigned long wifiTimeout=20000;
	bool sleep=false;
};

#endif