#include <commons/log.h>


void inicializarLog(char *rutaDeLog);



void inicializarLog(char *rutaDeLog, t_log loggerSinPantalla, t_log loggerConPantalla){


		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"FileSystem", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"FileSystem", true, LOG_LEVEL_INFO);

}
