/*
 * planificacion.h

 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "configuraciones.h"
#include "pcb.h"
#include "conexionMemoria.h"

typedef struct CPU {
	int pid;
	int socket;
}t_cpu;
int pid=0;
typedef struct CONSOLA{
	int pid;
	int socketHiloPrograma;
}t_consola;

typedef struct {
	char* codigo;
	int size;
	int pid;
	int socketHiloConsola;
}t_codigoPrograma;


/*---PAUSA PLANIFICACION---*/
int flagTerminarPlanificadorLargoPlazo = 1;
int flagPlanificacion = 1;

void reanudarPLanificacion();
void pausarPlanificacion();

void listaEsperaATerminados();
/*-------------------------*/

/*----LARGO PLAZO--------*/
void* planificarLargoPlazo();
void crearProceso(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
void informarConsola(int socketHiloPrograma,char* mensaje, int size);
t_codigoPrograma* recibirCodigoPrograma(int socketConsola);
t_codigoPrograma* buscarCodigoDeProceso(int pid);
t_list* listaCodigosProgramas;
pthread_t planificadorLargoPlazo;
/*----LARGO PLAZO--------*/

/*----CORTO PLAZO--------*/

void* planificarCortoPlazo();
void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex);
pthread_t planificadorCortoPlazo;
t_list* listaFinQuantum;
void finQuantumAReady();
void agregarAFinQuantum(t_pcb* pcb);
/*----CORTO PLAZO--------*/

/*---PLANIFICACION GENERAL-------*/
int atenderNuevoPrograma(int socketAceptado);
int verificarGradoDeMultiprogramacion();
void encolarProcesoListo(t_pcb *procesoListo);
void cargarConsola(int pid, int idConsola);
void terminarProceso(int socket);

int contadorPid=0;
t_list* colaNuevos;
t_list* colaListos;
t_list* colaTerminados;
t_list* colaEjecucion;
t_list* colaBloqueados;

t_list* listaConsolas;
t_list* listaCPU;
t_list* listaEnEspera;
/*---PLANIFICACION GENERAL-------*/


/*------------------------LARGO PLAZO-----------------------------------------*/
void* planificarLargoPlazo(int socket){
	t_pcb* proceso;
	while(flagTerminarPlanificadorLargoPlazo){
		sem_wait(&sem_admitirNuevoProceso);
			if(verificarGradoDeMultiprogramacion() == 0 && list_size(colaNuevos)>0 && flagPlanificacion) {
			log_info(loggerConPantalla,"Inicializando nuevo proceso desde cola de Nuevos");

			pthread_mutex_lock(&mutexColaNuevos);
			proceso = list_remove(colaNuevos,0);
			pthread_mutex_unlock(&mutexColaNuevos);

			t_codigoPrograma* codigoPrograma = buscarCodigoDeProceso(proceso->pid);
			crearProceso(proceso,codigoPrograma);
			}
	}
	log_info(loggerConPantalla,"Planificador largo palzo finalizado");
}

t_codigoPrograma* recibirCodigoPrograma(int socketHiloConsola){
	log_info(loggerConPantalla,"Recibiendo codigo del nuevo programa ANSISOP");
	t_codigoPrograma* codigoPrograma=malloc(sizeof(t_codigoPrograma));
	recv(socketHiloConsola,&codigoPrograma->size, sizeof(int),0);
	codigoPrograma->codigo = malloc(codigoPrograma->size);
	recv(socketHiloConsola,codigoPrograma->codigo,codigoPrograma->size,0);
	strcpy(codigoPrograma->codigo + codigoPrograma->size , "\0");
	codigoPrograma->socketHiloConsola=socketHiloConsola;
	return codigoPrograma;
}

t_codigoPrograma* buscarCodigoDeProceso(int pid){
	_Bool verificarPid(t_codigoPrograma* codigoPrograma){
			return (codigoPrograma->pid == pid);
		}
	return list_remove_by_condition(listaCodigosProgramas, (void*)verificarPid);

}

void crearProceso(t_pcb* proceso,t_codigoPrograma* codigoPrograma){
	if(inicializarProcesoEnMemoria(proceso,codigoPrograma) < 0 ){
				log_error(loggerConPantalla ,"No se pudo reservar recursos para ejecutar el programa");
				interruptHandler(codigoPrograma->socketHiloConsola,'A'); // Informa a consola error por no poder reservar recursos
				eliminarHiloPrograma(proceso->pid);
				free(proceso);
				free(codigoPrograma);
			}
	else{
			encolarProcesoListo(proceso);
			free(codigoPrograma);
			log_info(loggerConPantalla, "PCB encolado en cola de listos ---- PID: %d", proceso->pid);
			pthread_mutex_lock(&mutexGradoMultiProgramacion);
			gradoMultiProgramacion++;
			pthread_mutex_unlock(&mutexGradoMultiProgramacion);
			sem_post(&sem_colaReady);
	}
}

int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma){
	log_info(loggerConPantalla, "Inicializando proceso en memoria--->PID: %d", proceso->pid);
	if((pedirMemoria(proceso))< 0){
				log_error(loggerConPantalla ,"\nMemoria no autorizo la solicitud de reserva");
				return -1;
			}
	if((almacenarCodigoEnMemoria(proceso,codigoPrograma->codigo,codigoPrograma->size))< 0){
					log_error(loggerConPantalla ,"\nMemoria no puede almacenar contenido");
					return -2;
				}
	return 0;
}



/*------------------------LARGO PLAZO-----------------------------------------*/

void pausarPlanificacion(){

	flagPlanificacion = 0;
	sem_wait(&sem_planificacion);

}

void reanudarPLanificacion(){
	flagPlanificacion = 1;
	sem_post(&sem_planificacion);
}



/*------------------------CORTO PLAZO-----------------------------------------*/

void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex){

	pthread_mutex_lock(&mutex);
	list_add(lista, elemento);
	pthread_mutex_unlock(&mutex);
}

void* planificarCortoPlazo(){
	t_pcb* pcbListo;
	t_cpu* cpuEnEjecucion = malloc(sizeof(t_cpu));
	int quantum = 0; //SI ES 0 ES FIFO SINO ES UN QUANTUM
	char comandoEnviarPcb ='S';
	char comandoExpropiar ='E';

	if(!strcmp(config_algoritmo, "RR")) quantum = config_quantum;

	while(1){

		sem_wait(&sem_CPU);
		sem_wait(&sem_colaReady);
		sem_wait(&sem_planificacion);


		pthread_mutex_lock(&mutexColaListos);
		pcbListo = list_remove(colaListos,0);
		pthread_mutex_unlock(&mutexColaListos);


		pthread_mutex_lock(&mutexListaCPU);
		cpuEnEjecucion = list_remove(listaCPU,0);
		list_add_in_index(listaCPU, list_size(listaCPU), cpuEnEjecucion);
		pthread_mutex_unlock(&mutexListaCPU);

		cpuEnEjecucion->pid = pcbListo->pid;

		printf("\n\nPID:%d\n\n", cpuEnEjecucion->pid);

		pthread_mutex_lock(&mutexColaEjecucion);
		list_add(colaEjecucion, pcbListo);
		pthread_mutex_unlock(&mutexColaEjecucion);

		send(cpuEnEjecucion->socket,&comandoEnviarPcb,sizeof(char),0);
		send(cpuEnEjecucion->socket,&quantum,sizeof(int),0);

		serializarPcbYEnviar(pcbListo, cpuEnEjecucion->socket);

		finQuantumAReady();

		if(flagPlanificacion) sem_post(&sem_planificacion);

	}
}



void finQuantumAReady(){


	int indice;
	t_pcb* pcbBuffer;

	if(!list_is_empty(listaFinQuantum)){

		for (indice = 0; indice < list_size(listaFinQuantum)-1; indice++) {
				pthread_mutex_lock(&mutexListaFinQuantum);
				pcbBuffer = list_remove(listaFinQuantum,indice);
				pthread_mutex_unlock(&mutexListaFinQuantum);

				pthread_mutex_lock(&mutexColaListos);
				list_add(colaListos, pcbBuffer);
				pthread_mutex_unlock(&mutexColaListos);
				sem_post(&sem_colaReady);
		}
	}
}


void agregarAFinQuantum(t_pcb* pcb){

	_Bool verificarPidPcb(t_pcb* unPcb){
		return (unPcb->pid == pcb->pid);
	}

	pthread_mutex_lock(&mutexListaFinQuantum);
	list_add(listaFinQuantum, pcb);
	pthread_mutex_unlock(&mutexListaFinQuantum);

	pthread_mutex_lock(&mutexColaEjecucion);
	list_remove_by_condition(colaEjecucion, (void*)verificarPidPcb);
	pthread_mutex_unlock(&mutexColaEjecucion);

	sem_post(&sem_CPU);

}

/*------------------------CORTO PLAZO-----------------------------------------*/

/*-------------------PLANIFICACION GENERAL------------------------------------*/

int atenderNuevoPrograma(int socketAceptado){
		log_info(loggerConPantalla,"Atendiendo nuevo programa");

		contadorPid++; // VAR GLOBAL
		send(socketAceptado,&contadorPid,sizeof(int),0);

		t_codigoPrograma* codigoPrograma = recibirCodigoPrograma(socketAceptado);
		t_pcb* proceso=crearPcb(codigoPrograma->codigo,codigoPrograma->size);
		codigoPrograma->pid=proceso->pid;

		if(!flagPlanificacion) {
					contadorPid--;
					free(proceso);
					free(codigoPrograma);
					log_warning(loggerConPantalla,"La planificacion del sistema esta detenida");
					interruptHandler(socketAceptado,'D'); // Informa a consola error por planificacion detenida
					return -1;
						}
		list_add(colaNuevos,proceso);
		list_add(listaCodigosProgramas,codigoPrograma);
		cargarConsola(proceso->pid,codigoPrograma->socketHiloConsola);

		if(verificarGradoDeMultiprogramacion() < 0 ){
					interruptHandler(socketAceptado,'M'); // Informa a consola error por grado de multiprogramacion
					return -2;
				}
		else{
			sem_post(&sem_admitirNuevoProceso);
		}
		return 0;
}


int verificarGradoDeMultiprogramacion(){
	pthread_mutex_lock(&mutexGradoMultiProgramacion);
	if(gradoMultiProgramacion >= config_gradoMultiProgramacion) {
		pthread_mutex_unlock(&mutexGradoMultiProgramacion);
		log_error(loggerConPantalla, "Capacidad limite de procesos en sistema\n");
		return -1;
	}
	pthread_mutex_unlock(&mutexGradoMultiProgramacion);
	return 0;
}

void cargarConsola(int pid, int socketHiloPrograma) {
	t_consola *infoConsola = malloc(sizeof(t_consola));
	infoConsola->socketHiloPrograma=socketHiloPrograma;
	infoConsola->pid=pid;

	pthread_mutex_lock(&mutexListaConsolas);
	list_add(listaConsolas,infoConsola);
	pthread_mutex_unlock(&mutexListaConsolas);
}

void encolarProcesoListo(t_pcb *procesoListo){
	pthread_mutex_lock(&mutexColaListos);
	list_add(colaListos,procesoListo);
	pthread_mutex_unlock(&mutexColaListos);
}


void terminarProceso(int socketCPU){
	t_pcb* pcbProcesoTerminado;
	t_consola* consolaAInformar;

	_Bool verificarPidConsola(t_consola* consola){
						return (consola->pid == pcbProcesoTerminado->pid);
	}

	_Bool verificarPid(t_pcb* pcb){
		return (pcb->pid == pcbProcesoTerminado->pid);
	}

	_Bool verificarCPU(t_cpu* cpu){
			return (cpu->socket == socketCPU);
	}

	pcbProcesoTerminado = recibirYDeserializarPcb(socketCPU);
	log_info(loggerConPantalla, "Terminando proceso---- PID: %d ", pcbProcesoTerminado->pid);

	if(flagPlanificacion){



		pthread_mutex_lock(&mutexListaEspera);
		listaEsperaATerminados();
		pthread_mutex_unlock(&mutexListaEspera);

		pthread_mutex_lock(&mutexColaEjecucion);
		list_remove_by_condition(colaEjecucion, (void*)verificarPid);
		pthread_mutex_unlock(&mutexColaEjecucion);

		pthread_mutex_lock(&mutexColaTerminados);
		list_add(colaTerminados, pcbProcesoTerminado);
		pthread_mutex_unlock(&mutexColaTerminados);

		consolaAInformar = list_remove_by_condition(listaConsolas,(void*) verificarPidConsola);

		char* mensaje = "Finalizar";
		int size= strlen(mensaje);
		informarConsola(consolaAInformar->socketHiloPrograma,mensaje,size);

		pthread_mutex_lock(&mutexGradoMultiProgramacion);
		gradoMultiProgramacion--;
		pthread_mutex_unlock(&mutexGradoMultiProgramacion);
		sem_post(&sem_admitirNuevoProceso);

		sem_post(&sem_CPU);

	}else {

		pthread_mutex_lock(&mutexListaEspera);
		list_add(listaEnEspera, pcbProcesoTerminado);
		pthread_mutex_unlock(&mutexListaEspera);

		pthread_mutex_lock(&mutexGradoMultiProgramacion);
		gradoMultiProgramacion--;
		pthread_mutex_unlock(&mutexGradoMultiProgramacion);

		sem_post(&sem_CPU);

	}

	/* TODO:Liberar recursos */
	//free(consolaAInformar); //Ojo aca, si metes este free aca nomas, vas a ver que borras la consolaAInformar antes de que la Consola reciba el mensaje. Hay que hacer algun tipo de espera aca
}

void listaEsperaATerminados(){
	int indice;
	t_pcb* pcbBuffer;
	t_pcb* aux;

	if(!list_is_empty(listaEnEspera)){

		list_add_all(listaEnEspera, colaTerminados);

		for (indice = 0; indice < list_size(listaEnEspera)-1; indice++) {

			pcbBuffer = list_remove(listaEnEspera,indice);

			for (indice = 0; indice < list_size(colaEjecucion)-1; ++indice) {
				aux = list_get(colaEjecucion,indice);
				if(aux->pid == pcbBuffer->pid) list_remove(colaEjecucion, indice);

			}
		}
	}
}




void informarConsola(int socketHiloPrograma,char* mensaje, int size){
	send(socketHiloPrograma,&size,sizeof(int),0);
	send(socketHiloPrograma,mensaje,size,0);
}
#endif /* PLANIFICACION_H_ */
