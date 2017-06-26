/*
 * Excepciones.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef EXCEPECIONES_H_
#define EXCEPECIONES_H_
#define CANTIDADEXCEPCIONES 12
#include "conexionConsola.h"
#include "sockets.h"
#include "listasAdministrativas.h"

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
	EXIT_CREATE_PERMISSIONS,
	EXIT_MEMORY_EXCEPTION,
	EXIT_DISCONNECTED_CONSOLE,
	EXIT_END_OF_PROCESS,
	EXIT_PAGE_OVERSIZE,
	EXIT_PAGE_LIMIT,
	EXIT_STACKOVERFLOW
};
int resultadoEjecucion=-1;

t_exitCode* exitCodeArray [CANTIDADEXCEPCIONES];
void inicializarExitCodeArray();
void terminarProceso(t_pcb* pcb);
t_pcb* expropiarVoluntariamente(int socket);
t_pcb* expropiarPorEjecucion(int socket);
void cambiarEstadoATerminado(t_pcb* procesoTerminar);
void finalizarHiloPrograma(int pid);
void removerDeColaEjecucion(int pid);
void liberarRecursosEnMemoria(t_pcb* pcbProcesoTerminado);
void liberarMemoriaDinamica(int pid);


void excepcionReservaRecursos(int socketAceptado,t_pcb* pcb);
void excepcionPlanificacionDetenida(int socket);

/*TODO: Finalizar bien los procesos*/
void excepcionPermisosEscritura(int socket,int pid);
void excepcionPermisosLectura(int socket,int pid);
void excepecionPermisosCrear(int socket);
void excepcionArchivoInexistente(int socket,int pid);
void excepcionPageSizeLimit(int socket,int pid);
void excepcionStackOverflow(int socket);


void excepcionReservaRecursos(int socket,t_pcb* proceso){ /*TODO*/
	log_error(loggerConPantalla,"Informando a Consola excepcion por problemas al reservar recursos");
	informarConsola(socket,exitCodeArray[EXIT_RESOURCE]->mensaje,strlen(exitCodeArray[EXIT_RESOURCE]->mensaje));
	proceso->exitCode = exitCodeArray[EXIT_RESOURCE]->value;
	cambiarEstadoATerminado(proceso); /*TODO:Cambiar exitCODE*/
	finalizarHiloPrograma(proceso->pid);
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

void excepcionPermisosEscritura(int socketCPU,int pid){ /*TODO*/
	log_error(loggerConPantalla,"Informando a Consola excepcion por permisos de escritura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje));

}
void excepcionPermisosLecutra(int socket,int pid){ /*TODO*/
	log_error(loggerConPantalla,"Informando a Consola excepcion por permisos de lectura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje));

}

void excepecionPermisosCrear(int socket){
	log_error(loggerConPantalla,"Informando a Consola excepcion por permisos de creacion");
	t_pcb* proceso = expropiarVoluntariamente(socket);
	informarConsola(buscarSocketHiloPrograma(proceso->pid),exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje));
	proceso->exitCode = exitCodeArray[EXIT_CREATE_PERMISSIONS]->value;
	terminarProceso(proceso);
}

void excepcionArchivoInexistente(int socket,int pid){ /*TODO*/
	log_error(loggerConPantalla,"Informando a Consola excepcion por archivo inexistente");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje,strlen(exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje));

}

void excepcionPageSizeLimit(int socket,int pid){
	log_error(loggerConPantalla,"Informando a Consola excepecion de exceso de memoria dinamica");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje,strlen(exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_PAGE_OVERSIZE]->value;
	terminarProceso(proceso);
}

void excepcionStackOverflow(int socket){
	log_error(loggerConPantalla,"Informando a Consola excepcion por StackOverflow");
	t_pcb* pcb=recibirYDeserializarPcb(socket);
	informarConsola(buscarSocketHiloPrograma(pcb->pid),exitCodeArray[EXIT_STACKOVERFLOW]->mensaje,strlen(exitCodeArray[EXIT_STACKOVERFLOW]->mensaje));
	removerDeColaEjecucion(pcb->pid);
	pcb->exitCode =  exitCodeArray[EXIT_STACKOVERFLOW]->value;
	terminarProceso(pcb);
}

void terminarProceso(t_pcb* proceso){
	log_info(loggerConPantalla,"Liberando recursos--->PID:%d",proceso->pid);

	finalizarHiloPrograma(proceso->pid);
	liberarRecursosEnMemoria(proceso);
	liberarMemoriaDinamica(proceso->pid);
	cambiarEstadoATerminado(proceso);

	disminuirGradoMultiprogramacion();
	sem_post(&sem_admitirNuevoProceso);
}

t_pcb* expropiarVoluntariamente(int socket){
	t_pcb* pcb;
	log_info(loggerConPantalla,"Expropiando pcb---CPU:%d",socket);
	char comandoExpropiar='F';

	send(socket,&comandoExpropiar,sizeof(char),0);
	pcb = recibirYDeserializarPcb(socket);
	removerDeColaEjecucion(pcb->pid);
	return pcb;
}

t_pcb* expropiarPorEjecucion(int socket){
	t_pcb* pcb;
	int resultadoEjecucion=-1;
	log_info(loggerConPantalla,"Expropiando pcb---CPU:%d",socket);
		send(socket,&resultadoEjecucion,sizeof(char),0);
		pcb = recibirYDeserializarPcb(socket);
		removerDeColaEjecucion(pcb->pid);
		return pcb;
}
void cambiarEstadoATerminado(t_pcb* procesoTerminar){
	_Bool verificaPid(t_pcb* pcb){
			return (pcb->pid == procesoTerminar->pid);
		}

	pthread_mutex_lock(&mutexColaTerminados);
	list_add(colaTerminados,procesoTerminar);
	pthread_mutex_unlock(&mutexColaTerminados);
}
void finalizarHiloPrograma(int pid){
	char* mensaje = malloc(sizeof(char)*10);

	t_consola* consola = malloc(sizeof(t_consola));
	mensaje = "Finalizar";

	_Bool verificaPid(t_consola* consolathread){
			return (consolathread->pid == pid);
	}
		pthread_mutex_lock(&mutexListaConsolas);
		consola = list_remove_by_condition(listaConsolas,(void*)verificaPid);
		pthread_mutex_unlock(&mutexListaConsolas);

		informarConsola(consola->socketHiloPrograma,mensaje,strlen(mensaje));
		eliminarSocket(consola->socketHiloPrograma);


	//free(mensaje); TODO: Ver porque rompe este free;
	free(consola);
}


void removerDeColaEjecucion(int pid){
	_Bool verificaPid(t_pcb* proceso){
			return (proceso->pid == pid);
		}
	pthread_mutex_lock(&mutexColaEjecucion);
	list_remove_by_condition(colaEjecucion, (void*)verificaPid);
	pthread_mutex_unlock(&mutexColaEjecucion);
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

	exitCodeArray[EXIT_CREATE_PERMISSIONS]->value = -5;
	exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje= "El programa intento crear un archivo sin permisos";


	exitCodeArray[EXIT_MEMORY_EXCEPTION]->value = -6;
	exitCodeArray[EXIT_MEMORY_EXCEPTION]->mensaje= "Excepcion de memoria";

	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->value = -7;
	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->mensaje= "Finalizado a traves de desconexion de consola";

	exitCodeArray[EXIT_END_OF_PROCESS]->value = -8;
	exitCodeArray[EXIT_END_OF_PROCESS]->mensaje= "Finalizado a traves del comando Finalizar Programa";

	exitCodeArray[EXIT_PAGE_OVERSIZE]->value = -9;
	exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje= "Se intento reservar mas memoria que el tamano de una pagina";

	exitCodeArray[EXIT_PAGE_LIMIT]->value = -10;
	exitCodeArray[EXIT_PAGE_LIMIT]->mensaje= "No se pueden asignar mas paginas al proceso";

	exitCodeArray[EXIT_STACKOVERFLOW]->value = -11;
	exitCodeArray[EXIT_STACKOVERFLOW]->mensaje= "El programa sufrio StackOverflow";


}
#endif /* EXCEPECIONES_H_ */
