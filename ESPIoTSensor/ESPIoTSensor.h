#ifndef ESPIoTSensor_h
#define ESPIoTSensor_h

#define MAX_PARA_LEN 64

struct params {
	
	char ssid[MAX_PARA_LEN] = "some-ssid";
	char password[MAX_PARA_LEN] = "some-passwd";
	char INFLUXDB_HOST[MAX_PARA_LEN] = "some-host";
	uint16_t INFLUXDB_PORT = 8086;
	char DATABASE[MAX_PARA_LEN] = "some-database";
	char DB_USER[MAX_PARA_LEN] = "some-database-user";
	char DB_PASSWORD[MAX_PARA_LEN] = "some-database-password";
	float sleepTime=30;
	unsigned long wifi_timeout=20000;
	bool sleep=false;
};

#endif