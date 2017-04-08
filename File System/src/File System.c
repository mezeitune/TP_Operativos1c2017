/*
 ============================================================================
 Name        : File.c
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
char *puerto;
char *puntoMontaje;


//------------------Configs----------------------//
void leerConfiguracion(char* ruta);
//-----------------------------------------------//


int main(void)
{

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/\"File System\"/config_FileSys");


	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta){

	puerto = config_get_int_value(configuracion_kernel,"PUERTO");
	puntoMontaje = config_get_string_value(configuracion_kernel,"PUNTO_MENSAJE");



}

