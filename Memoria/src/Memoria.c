/*
 ============================================================================
 Name        : Memoria.c
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

//#define RUTA_LOG "/home/utnso/memoria.log"

t_config* configuracion_memoria;
int puertoMemoria;
int marcos;
int marco_size;
int entradas_cache;
int cache_x_proc;
int retardo_memoria;

//Revisar y discutir estructuras

typedef struct
{
	char datosMarco[marco_size];
	//Revisar
}t_Memoria;

typedef struct
{
	int marco;
	int pid;
	int num_pag;
}struct_adm_memoria;

void leerConfiguracion(char* ruta);//Revisar junto a estructura
void inicializarMemoria(char* ruta);//Falta codificar

//-----------------------FUNCIONES MEMORIA--------------------------//
int inicializarPrograma(int pid, int cantPaginas);//Falta codificar
int solicitarBytesPagina(int pid,int pagina, int offset, int size);//Falta codificar
int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer);//Falta codificar
int asignarPaginaAProceso(int pid, int cantPaginas);//Falta codificar
int finalizarPrograma(int pid);//Falta codificar
//------------------------------------------------------------------//

int main(void)
{
	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta)
{
	configuracion_memoria = config_create(ruta);
	puertoMemoria = config_get_int_value(configuracion_memoria,"PUERTO");
	marcos = config_get_int_value(configuracion_memoria,"MARCOS");
	marco_size = config_get_int_value(configuracion_memoria,"MARCO_SIZE");
	entradas_cache = config_get_int_value(configuracion_memoria,"ENTRADA_CACHE");
	cache_x_proc = config_get_int_value(configuracion_memoria,"CACHE_X_PROC");
	retardo_memoria = config_get_int_value(configuracion_memoria,"RETARDO_MEMORIA");
}

void inicializarMemoria(char* ruta)
{

}

int inicializarPrograma(int pid, int cantPaginas)
{
	return EXIT_SUCCESS;
}

int solicitarBytesPagina(int pid,int pagina, int offset, int size)
{
	return EXIT_SUCCESS;
}

int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer)
{
	return EXIT_SUCCESS;
}

int asignarPaginaAProceso(int pid, int cantPaginas)
{
	return EXIT_SUCCESS;
}

int finalizarPrograma(int pid)
{
	return EXIT_SUCCESS;
}
