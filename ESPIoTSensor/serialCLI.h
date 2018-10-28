#ifndef serialCLI_h
#define serialCLI_h

#define LINE_BUF_SIZE 128   //Maximum input string length
#define ARG_BUF_SIZE 64     //Maximum argument string length
#define MAX_NUM_ARGS 8      //Maximum number of arguments

//Function declarations
int cmd_help();
void help_help();

void cli_init();
void my_cli();

enum para_type {pt_string, pt_int, pt_float, pt_uint16, pt_ulong, pt_bool};

typedef struct{
	const char *parameter;
	union {	
		char *pstr;
		float *pfloat;
		uint16_t *puint16;
		int *pint;
		unsigned long *pulong; 
		bool *pbool;
	};
	const para_type type;
} strSetPara;

typedef struct{
	const char *cmd;
	void (*help_ptr)();
	int (*exec_ptr)();
} strCmd;


#endif
