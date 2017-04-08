/*
 ============================================================================
 Name        : Consola.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>

t_config* configuracion_Consola;
char* ip_Kernel;
int puerto_Kernel;

void leerConfiguracion(char* ruta);
int main(void)
{

	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta)
{
	configuracion_Consola= config_create(ruta);
	ip_Kernel= config_get_string_value(configuracion_Consola,"IP_KERNEL");
	puerto_Kernel = config_get_int_value(configuracion_Consola,"PUERTO_KERNEL");
}
