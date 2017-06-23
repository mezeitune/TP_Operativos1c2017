/*
 * Excepciones.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef EXCEPCIONES_H_
#define EXCEPCIONES_H_
#define CANTIDADEXCEPCIONES 10
#include "conexionConsola.h"


typedef struct{
	int value;
	char* mensaje;
}t_exitCode;

enum {
	EXIT_OK,
	EXIT_RESOURCE,
	EXIT_FILE_NOT_FOUND,
	EXIT_READ_PERMISSIONS,
	EXIT_WRITE_PERMISSIONS,
	EXIT_MEMORY_EXCEPTION,
	EXIT_DISCONNECTED_CONSOLE,
	EXIT_END_OF_PROCESS,
	EXIT_PAGE_OVERSIZE,
	EXIT_PAGE_LIMIT
};


t_exitCode* exitCodeArray [CANTIDADEXCEPCIONES];

void excepcionReservaRecursos(int socketAceptado);
void excepcionPlanificacionDetenida(int socket);

/*TODO: Finalizar bien los procesos*/
void excepcionPermisosEscritura(int socket,int pid);
void excepcionPermisosLectura(int socket,int pid);
void excepcionArchivoInexistente(int socket,int pid);

void excepcionReservaRecursos(int socket){
	log_error(loggerConPantalla,"Informando a Consola excepcion por problemas al reservar recursos");
	informarConsola(socket,exitCodeArray[EXIT_RESOURCE]->mensaje,strlen(exitCodeArray[EXIT_RESOURCE]->mensaje));
	log_error(loggerConPantalla,"El programa ANSISOP enviado por socket: %d ha sido expulsado del sistema e se ha informado satifactoriamente",socket);
}

void excepcionPlanificacionDetenida(int socket){
	log_error(loggerConPantalla,"Informando a Consola excepcion por planificacion detenido");
	informarConsola(socket,exitCodeArray[EXIT_RESOURCE]->mensaje,strlen(exitCodeArray[EXIT_RESOURCE]->mensaje));
	char* mensaje = "Finalizar";
	int size=strlen(mensaje);
	informarConsola(socket,mensaje,size);
	recv(socket,&size,sizeof(int),0); // A modo de ok
	eliminarSocket(socket);
}

void excepcionPermisosEscritura(int socketCPU,int pid){
	log_error(loggerConPantalla,"Informando a Consola excepcion por permisos de escritura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje));

}
void excepcionPermisosLecutra(int socket,int pid){
	log_error(loggerConPantalla,"Informando a Consola excepcion por permisos de lectura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje));

}
void excepcionArchivoInexistente(int socket,int pid){
	log_error(loggerConPantalla,"Informando a Consola excepcion por archivo inexistente");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje,strlen(exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje));

}


#endif /* EXCEPCIONES_H_ */
