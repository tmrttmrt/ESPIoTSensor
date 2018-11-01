#ifndef ESPIoTSensor_h
#define ESPIoTSensor_h

#define MAX_PARA_LEN 64
#define TIME_FOR_WIFI_CONFIG 10*60*1000
#define AP_SSID "ESPIoTSensor"
#define AP_PWD ""
const byte DNS_PORT = 53;

struct settings {
	
	char ssid[MAX_PARA_LEN] = "some-ssid";
	char password[MAX_PARA_LEN] = "some-passwd";
	char influxDBHost[MAX_PARA_LEN] = "some-host";
	uint16_t influxDBPort = 8086;
	char dbName[MAX_PARA_LEN] = "some-dbName";
	char dbUser[MAX_PARA_LEN] = "some-dbase-user";
	char dbPasswd[MAX_PARA_LEN] = "some-dbase-password";
	char sensName[MAX_PARA_LEN] = "sensor-name";
	char dbLocationTag[MAX_PARA_LEN] = "some-location";
	unsigned long sleepTime=30;
	unsigned long wifiTimeout=20000;
	bool sleep=false;
};

void save_settings();

#endif