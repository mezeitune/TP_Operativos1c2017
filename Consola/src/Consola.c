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
char* ipKernel;
char* puertoKernel;

void leerConfiguracion(char* ruta);
int main(void)
{

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");


	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta)
{
	configuracion_Consola = config_create(ruta);
	printf("%s",ipKernel = config_get_string_value(configuracion_Consola,"IP_KERNEL"));
	puertoKernel = config_get_string_value(configuracion_Consola,"PUERTO_KERNEL");
}
