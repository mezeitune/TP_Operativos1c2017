/*
 * Excepciones.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef EXCEPCIONES_H_
#define EXCEPCIONES_H_
#define CANTIDADEXCEPCIONES 11
#include "conexionConsola.h"
#include "planificacion.h"


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
	EXIT_PAGE_LIMIT,
	EXIT_STACKOVERFLOW
};


t_exitCode* exitCodeArray [CANTIDADEXCEPCIONES];
void inicializarExitCodeArray();
void terminarProceso(t_pcb* pcb,int exitCode);
void excepcionReservaRecursos(int socketAceptado);
void excepcionPlanificacionDetenida(int socket);

/*TODO: Finalizar bien los procesos*/
void excepcionPermisosEscritura(int socket,int pid);
void excepcionPermisosLectura(int socket,int pid);
void excepcionArchivoInexistente(int socket,int pid);
void excepcionStackOverflow(int socket);


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

void excepcionStackOverflow(int socket){
	log_error(loggerConPantalla,"Informando a Consola excepcion por StackOverflow");
	t_pcb* pcb=recibirYDeserializarPcb(socket);
	informarConsola(buscarSocketHiloPrograma(pcb->pid),exitCodeArray[EXIT_STACKOVERFLOW]->mensaje,strlen(exitCodeArray[EXIT_STACKOVERFLOW]->mensaje));
	/*TODO: Sacar de la cola de ejecucion*/
	terminarProceso(pcb,exitCodeArray[EXIT_STACKOVERFLOW]->value);
}

void terminarProceso(t_pcb* proceso,int exitCode){
	finalizarHiloPrograma(proceso->pid);
	liberarRecursosEnMemoria(proceso);
	cambiarEstadoATerminado(proceso,exitCodeArray[EXIT_STACKOVERFLOW]->value);
}


void inicializarExitCodeArray(){
	int i;
	for(i = 0 ; i< CANTIDADEXCEPCIONES ; i++){
		exitCodeArray [i] = malloc (sizeof(t_exitCode));
	}

	exitCodeArray[EXIT_OK]->value = 0;
	exitCodeArray[EXIT_OK]->mensaje= "Programa finalizado exitosamente";

	exitCodeArray[EXIT_RESOURCE]->value = -1;
	exitCodeArray[EXIT_RESOURCE]->mensaje= "No se puedieron reservar recursos para ejecutar el programa";

	exitCodeArray[EXIT_FILE_NOT_FOUND]->value = -2;
	exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje= "El programa intento acceder a un archivo que no existe";

	exitCodeArray[EXIT_READ_PERMISSIONS]->value = -3;
	exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje= "El programa intento leer un archivo sin permisos";

	exitCodeArray[EXIT_WRITE_PERMISSIONS]->value = -4;
	exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje= "El programa intento escribir un archivo sin permisos";

	exitCodeArray[EXIT_MEMORY_EXCEPTION]->value = -5;
	exitCodeArray[EXIT_MEMORY_EXCEPTION]->mensaje= "Excepcion de memoria";

	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->value = -6;
	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->mensaje= "Finalizado a traves de desconexion de consola";

	exitCodeArray[EXIT_END_OF_PROCESS]->value = -7;
	exitCodeArray[EXIT_END_OF_PROCESS]->mensaje= "Finalizado a traves del comando Finalizar Programa";

	exitCodeArray[EXIT_PAGE_OVERSIZE]->value = -8;
	exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje= "Se intento reservar mas memoria que el tamano de una pagina";

	exitCodeArray[EXIT_PAGE_LIMIT]->value = -9;
	exitCodeArray[EXIT_PAGE_LIMIT]->mensaje= "No se pueden asignar mas paginas al proceso";

	exitCodeArray[EXIT_PAGE_LIMIT]->value = -10;
	exitCodeArray[EXIT_PAGE_LIMIT]->mensaje= "El programa sufrio StackOverflow";
}
#endif /* EXCEPCIONES_H_ */
