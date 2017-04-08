/*
 ============================================================================
 Name        : Kernel.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>



t_config* configuracion_kernel;

char *ipMemoria;
char *puertoProg;//2001
char *puertoCPU;//3001
char *puertoMemoria;//4001
char *ipFileSys;
char *puertoFileSys;
char *quantum;
char *quantumSleep;
char *algoritmo;
char *gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;

int main(void){

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");


	return EXIT_SUCCESS;
}



void leerConfiguracion(char* ruta){

	configuracion_kernel = config_create(ruta);
	puertoProg = config_get_int_value(configuracion_kernel,"PUERTO_PROG");
	puertoCPU = config_get_int_value(configuracion_kernel,"IP_CPU");
	ipMemoria = config_get_string_value(configuracion_kernel,"IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel,"IP_FS");
	puertoFileSys = config_get_int_value(configuracion_kernel,"PUERTO_FS");
	quantum = config_get_string_value(configuracion_kernel,"QUANTUM");
	quantumSleep = config_get_int_value(configuracion_kernel,"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion_kernel,"ALGORTIMO");
	gradoMultiProg = config_get_int_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION");
	semIds = config_get_array_value(configuracion_kernel,"SEM_IDS");
	semInit = config_get_array_value(configuracion_kernel,"SEM_INIT");
	sharedVars = config_get_array_value(configuracion_kernel,"SHARED_VARS");
}
