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
#include "contabilidad.h"
#include "semaforosAnsisop.h"
#include "comandosCPU.h"
#include "heap.h"
#include "Excepciones.h"


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
int flagTerminarPlanificadorLargoPlazo = 0;
int flagPlanificacion;

void verificarPausaPlanificacion();


void reanudarPLanificacion();
void pausarPlanificacion();

void listaEsperaATerminados();
/*-------------------------*/

/*----LARGO PLAZO--------*/
void planificarLargoPlazo();
void administrarNuevosProcesos();
void administrarFinProcesos();
void crearProceso(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
void informarConsola(int socketHiloPrograma,char* mensaje, int size);
t_codigoPrograma* buscarCodigoDeProceso(int pid);
void liberarRecursosEnMemoria(t_pcb* pcbProcesoTerminado);
void liberarMemoriaDinamica(int pid,int cantiPaginasCodigo);
t_list* listaCodigosProgramas;
pthread_t planificadorLargoPlazo;
/*----LARGO PLAZO--------*/

/*----MEDIANO PLAZO--------*/

void planificarMedianoPlazo();
void pcbBloqueadoAReady();
void pcbEjecucionABloqueado();
pthread_t planificadorMedianoPlazo;

/*----MEDIANO PLAZO--------*/

/*----CORTO PLAZO--------*/

void planificarCortoPlazo();
void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex);
void finQuantumAReady();
void agregarAFinQuantum(t_pcb* pcb);

t_list* listaFinQuantum;
pthread_t planificadorCortoPlazo;
pthread_t threadFinQuantumAReady;

/*----CORTO PLAZO--------*/

/*---PLANIFICACION GENERAL-------*/
int obtenerPaginaSiguiente(int pid);
int verificarGradoDeMultiprogramacion();
void aumentarGradoMultiprogramacion();
void disminuirGradoMultiprogramacion();
void encolarProcesoListo(t_pcb *procesoListo);
void cargarConsola(int pid, int idConsola);
void gestionarFinalizacionProgramaEnCpu(int socket);
void cambiarEstadoATerminado(t_pcb* procesoTerminar,int exit);
void finalizarHiloPrograma(int pid);
void eliminarSocket(int socket);

int contadorPid=0;
t_list* colaNuevos;
t_list* colaListos;
t_list* colaTerminados;
t_list* colaEjecucion;
t_list* colaBloqueados;

t_list* listaConsolas;

t_list* listaEnEspera;

/*---PLANIFICACION GENERAL-------*/


/*------------------------LARGO PLAZO-----------------------------------------*/
void planificarLargoPlazo(){
	pthread_t administradorFinProcesos;
	pthread_t administradorNuevosProcesos;
	pthread_create(&administradorFinProcesos,NULL, (void*)administrarFinProcesos,NULL);
	pthread_create(&administradorNuevosProcesos,NULL, (void*)administrarNuevosProcesos,NULL);

	pthread_join(administradorNuevosProcesos,NULL);
	pthread_join(administradorFinProcesos,NULL);
	log_info(loggerConPantalla,"Planificador largo plazo finalizado");
}

void administrarNuevosProcesos(){
	t_pcb* proceso;
	while(!flagTerminarPlanificadorLargoPlazo){
			sem_wait(&sem_admitirNuevoProceso);
			pthread_mutex_lock(&mutexNuevoProceso);
				if(verificarGradoDeMultiprogramacion()==0 &&list_size(colaNuevos)>0 && flagPlanificacion) {

				pthread_mutex_lock(&mutexColaNuevos);
				proceso = list_remove(colaNuevos,0);
				pthread_mutex_unlock(&mutexColaNuevos);

				pthread_mutex_lock(&mutexListaCodigo);
				t_codigoPrograma* codigoPrograma = buscarCodigoDeProceso(proceso->pid);
				pthread_mutex_unlock(&mutexListaCodigo);

				crearProceso(proceso,codigoPrograma);
				}
				pthread_mutex_unlock(&mutexNuevoProceso);
		}
}

void administrarFinProcesos(){

	t_pcb* proceso;
	t_cpu* cpu;
	char ok;
	_Bool verificarPidConsola(t_consola* consola){
				return (consola->pid == proceso->pid);
					}

	_Bool verificaPid(t_pcb* pcb){
		return (pcb->pid == proceso->pid);
	}
	_Bool verificaCpu(t_cpu* cpu){
			return (cpu->enEjecucion == 2);
		}

	while(!flagTerminarPlanificadorLargoPlazo){
		sem_wait(&sem_administrarFinProceso);

			pthread_mutex_lock(&mutexListaCPU);
			cpu=list_remove_by_condition(listaCPU,(void*)verificaCpu);
			send(cpu->socket,&ok,sizeof(char),0);
			proceso= recibirYDeserializarPcb(cpu->socket);
			cpu->enEjecucion=0;
			list_add(listaCPU,cpu);
			pthread_mutex_unlock(&mutexListaCPU);

			sem_post(&sem_CPU);

			log_warning(loggerConPantalla, "Terminando proceso exitos desde CPU:%d--->PID:%d", cpu->socket,proceso->pid);
			actualizarRafagas(proceso->pid,proceso->cantidadInstrucciones);

				if(flagPlanificacion){

					listaEsperaATerminados();

					pthread_mutex_lock(&mutexColaEjecucion);
					list_remove_by_condition(colaEjecucion, (void*)verificaPid);
					pthread_mutex_unlock(&mutexColaEjecucion);

					cambiarEstadoATerminado(proceso,EXIT_OK); /*TODO: Cambiar exitCode*/
					disminuirGradoMultiprogramacion();
					sem_post(&sem_admitirNuevoProceso);

					finalizarHiloPrograma(proceso->pid);
					liberarRecursosEnMemoria(proceso);
					//liberarMemoriaDinamica(proceso->pid,proceso->cantidadPaginasCodigo);
					/*TODO: Ver que terminar de FS*/
					log_info(loggerConPantalla, "Proceso terminado--->PID:%d", proceso->pid);
				}else {

					pthread_mutex_lock(&mutexListaEspera);
					list_add(listaEnEspera, proceso);
					pthread_mutex_unlock(&mutexListaEspera);
					/*No hay que disminuir aca lo diminuyo cuando lo saco de ejecucion*/
				}

	}
}

void liberarMemoriaDinamica(int pid,int cantPaginasCodigo){
	int cantPaginasHeap;
	int numeroPagina;
	_Bool verificaPid(t_contable* proceso){
		return proceso->pid == pid;
	}

	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPid);
	cantPaginasHeap = proceso->cantPaginasHeap;
	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);

	while (cantPaginasHeap > 0){
		numeroPagina = stackSize + cantPaginasCodigo + cantPaginasHeap;
		destruirPaginaHeap(pid,numeroPagina);
		cantPaginasHeap --;
	}
}



t_codigoPrograma* buscarCodigoDeProceso(int pid){
	_Bool verificarPid(t_codigoPrograma* codigoPrograma){
			return (codigoPrograma->pid == pid);
		}

	return list_remove_by_condition(listaCodigosProgramas, (void*)verificarPid);

}

void crearProceso(t_pcb* proceso,t_codigoPrograma* codigoPrograma){
	log_info(loggerConPantalla,"Cambiando nuevo proceso desde Nuevos a Listos--->PID: %d",proceso->pid);
	if(inicializarProcesoEnMemoria(proceso,codigoPrograma) < 0 ){
				log_error(loggerConPantalla ,"No se pudo reservar recursos para ejecutar el programa");
				interruptHandler(codigoPrograma->socketHiloConsola,'A'); // Informa a consola error por no poder reservar recursos
				cambiarEstadoATerminado(proceso,-1); /*TODO:Cambiar exitCODE*/
				finalizarHiloPrograma(proceso->pid);
			}
	else{
			encolarProcesoListo(proceso);
			crearInformacionContable(proceso->pid);
			aumentarGradoMultiprogramacion();
			sem_post(&sem_colaListos);
	}
	free(codigoPrograma);
}

void cambiarEstadoATerminado(t_pcb* procesoTerminar,int exit){
	_Bool verificaPid(t_pcb* pcb){
			return (pcb->pid == procesoTerminar->pid);
		}

	procesoTerminar->exitCode=exit;
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

int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma){
	log_info(loggerConPantalla, "Inicializando proceso en memoria--->PID: %d", proceso->pid);
	if((pedirMemoria(proceso))< 0){
				log_error(loggerConPantalla ,"Memoria no autorizo la solicitud de reserva--->PID:%d",proceso->pid);
				return -1;
			}
	if((almacenarCodigoEnMemoria(proceso,codigoPrograma->codigo,codigoPrograma->size))< 0){
					log_error(loggerConPantalla ,"Memoria no puede almacenar contenido--->PID:%d",proceso->pid);
					return -2;
				}
	return 0;
}



/*------------------------LARGO PLAZO-----------------------------------------*/

void pausarPlanificacion(){

	if(!flagPlanificacion)log_warning(loggerConPantalla, "Planificacion ya pausada");
	else{
		flagPlanificacion = 0;
		sem_wait(&sem_planificacion);
		log_info(loggerConPantalla, "Se pauso la planificacion");
	}
}

void reanudarPLanificacion(){

	if(flagPlanificacion) log_warning(loggerConPantalla, "Planificacion no se encuentra pausada");
	else{
		flagPlanificacion = 1;

		sem_post(&sem_planificacion);
		sem_post(&sem_planificacion);
		sem_post(&sem_planificacion);

		log_info(loggerConPantalla, "Se reanudo la planificacion");
	}
}

void verificarPausaPlanificacion(){
	if(flagPlanificacion) sem_post(&sem_planificacion);
	sem_wait(&sem_planificacion);
}

/*------------------------CORTO PLAZO-----------------------------------------*/

void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex){

	pthread_mutex_lock(&mutex);
	list_add(lista, elemento);
	pthread_mutex_unlock(&mutex);
}

void planificarCortoPlazo(){
	t_pcb* pcbListo;
	t_cpu* cpuEnEjecucion = malloc(sizeof(t_cpu));

	_Bool verificarCPU(t_cpu* cpu){
		return (cpu->enEjecucion == 0);
	}
	char comandoEnviarPcb = 'S';


	pthread_create(&threadFinQuantumAReady, NULL, (void*)finQuantumAReady, NULL);


	while(1){

		verificarPausaPlanificacion();

		sem_wait(&sem_CPU);
		sem_wait(&sem_colaListos);


		pthread_mutex_lock(&mutexColaListos);
		pcbListo = list_remove(colaListos,0);
		pthread_mutex_unlock(&mutexColaListos);


		pthread_mutex_lock(&mutexListaCPU);
		if(list_any_satisfy(listaCPU, (void*) verificarCPU)){

			cpuEnEjecucion = list_remove_by_condition(listaCPU,(void*) verificarCPU);
			cpuEnEjecucion->enEjecucion = 1;
			list_add(listaCPU, cpuEnEjecucion);
			pthread_mutex_unlock(&mutexListaCPU);

			cpuEnEjecucion->pid = pcbListo->pid;

			pthread_mutex_lock(&mutexColaEjecucion);
			list_add(colaEjecucion, pcbListo);
			pthread_mutex_unlock(&mutexColaEjecucion);

			send(cpuEnEjecucion->socket,&comandoEnviarPcb,sizeof(char),0);

			serializarPcbYEnviar(pcbListo, cpuEnEjecucion->socket);
			log_info(loggerConPantalla,"Pcb encolado en Ejecucion--->PID:%d",pcbListo->pid);
		}

	}
}

void finQuantumAReady(){

	int indice;
	t_pcb* pcbBuffer;


	while(1){

		verificarPausaPlanificacion();

		sem_wait(&sem_listaFinQuantum);

		pthread_mutex_lock(&mutexListaFinQuantum);
			for (indice = 0; indice <= list_size(listaFinQuantum); ++indice) {

				pthread_mutex_unlock(&mutexListaFinQuantum);

				pcbBuffer = list_remove(listaFinQuantum,indice);

				pthread_mutex_lock(&mutexColaListos);
				list_add_in_index(colaListos, list_size(colaListos),pcbBuffer);
				pthread_mutex_unlock(&mutexColaListos);

				sem_post(&sem_colaListos);
			}
		pthread_mutex_unlock(&mutexListaFinQuantum);
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

	sem_post(&sem_listaFinQuantum);


}

/*------------------------CORTO PLAZO-----------------------------------------*/








/*------------------------MEDIANO PLAZO-----------------------------------------*/

pthread_t threadId;

 void planificarMedianoPlazo(){

	 pthread_create(&threadId,NULL, (void*)pcbBloqueadoAReady, NULL);

	 while(1){
		verificarPausaPlanificacion();
		pcbEjecucionABloqueado();

	}
 }


 /***************************************
 typedef struct{
 	t_semaforo* semaforo;
 	t_list* pids;
 }t_semaforoAsociado;
 ******************************************/



void pcbBloqueadoAReady(){

	int i;

	t_semYPCB *semYPCB;
	t_pcb *pcbADesbloquear;
	t_semaforo *semaforoBuffer;

	_Bool verificaSemId(t_semYPCB *semYPCBBuffer){
		return (!strcmp(semYPCBBuffer->idSemaforo, semaforoBuffer/*->semaforo*/->id));
	}

	_Bool verificaIdPCB(t_pcb *pcbBuffer){
			 return (pcbBuffer->pid == semYPCB->pcb->pid);
	}

	while(1){
		verificarPausaPlanificacion();
		sem_wait(&sem_semAumentados);

		for(i = 0; i < list_size(listaSemaforosGlobales); ++i) {

			pthread_mutex_lock(&mutexListaSemaforos);
			semaforoBuffer = list_get(listaSemaforosGlobales,i);
			pthread_mutex_unlock(&mutexListaSemaforos);

			if(semaforoBuffer/*->semaforo*/->valor > 0){


				pthread_mutex_lock(&mutexListaSemYPCB);
				if(list_any_satisfy(listaSemYPCB, (void*)verificaSemId)){

					semYPCB = list_remove_by_condition(listaSemYPCB, (void*)verificaSemId);
					pthread_mutex_unlock(&mutexListaSemYPCB);

					pthread_mutex_lock(&mutexColaBloqueados);
					if(list_any_satisfy(colaBloqueados, (void*)verificaIdPCB)){

						pcbADesbloquear = list_remove_by_condition(colaBloqueados,(void*)verificaIdPCB);
						pthread_mutex_unlock(&mutexColaBloqueados);

						pthread_mutex_lock(&mutexColaListos);
						list_add(colaListos,pcbADesbloquear);
						pthread_mutex_unlock(&mutexColaListos);

						sem_post(&sem_colaListos);
					}
				pthread_mutex_unlock(&mutexColaBloqueados);
				}
			pthread_mutex_unlock(&mutexListaSemYPCB);
			}
		}
	}
}


/*		 pthread_mutex_lock(&mutexListaSemAumentados);
		 if(!list_is_empty(listaSemAumentados)){
			 for(indice = 0; indice < list_size(listaSemAumentados); indice++) {

				 semIdBuffer = list_get(listaSemAumentados, indice);
				 printf("\n\nSEMIDBUFFER: %s\n\n", semIdBuffer);

				 pthread_mutex_lock(&mutexListaSemYPCB);
				 if(list_any_satisfy(listaSemYPCB, (void*)verificaSemId)){

					 semYPCB = list_remove_by_condition(listaSemYPCB,(void*)verificaSemId);

					 list_remove(listaSemAumentados, indice);

					 pthread_mutex_lock(&mutexColaBloqueados);
					 pcbADesbloquear = list_remove_by_condition(colaBloqueados, (void*)verificaIdPCB);
					 pthread_mutex_unlock(&mutexColaBloqueados);

					 log_info(loggerConPantalla,"Cambiando proceso desde Bloqueados a Listos--->PID:%d",pcbADesbloquear->pid);

					 pthread_mutex_lock(&mutexColaListos);
					 list_add(colaListos, pcbADesbloquear);
					 sem_post(&sem_colaReady);
					 pthread_mutex_unlock(&mutexColaListos);
				 }
				 pthread_mutex_unlock(&mutexListaSemYPCB);
			 }
		 }

		 pthread_mutex_unlock(&mutexListaSemAumentados);
	}*/
	 /*free(semIdBuffer);
	 free(semYPCB);
	 free(pcbADesbloquear);
}*/

void pcbEjecucionABloqueado(){

	int indice;
	t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));


	_Bool verificaPCB(t_pcb* pcb){
		return (pcb->pid == semYPCB->pcb->pid);
	}



	sem_wait(&sem_ListaSemYPCB);


	printf("\nBloqueando Proceso\n");

	pthread_mutex_lock(&mutexListaSemYPCB);
	for (indice = 0; indice < list_size(listaSemYPCB); ++indice) {

		semYPCB = list_get(listaSemYPCB, indice);
		pthread_mutex_unlock(&mutexListaSemYPCB);

		log_info(loggerConPantalla,"Cambiando proceso desde Ejecutados a Bloqueados--->PID:%d",semYPCB->pcb->pid);

		pthread_mutex_lock(&mutexColaEjecucion);
		list_remove_by_condition(colaEjecucion, (void*)verificaPCB);
		pthread_mutex_unlock(&mutexColaEjecucion);

		pthread_mutex_lock(&mutexColaBloqueados);
		list_add(colaBloqueados, semYPCB->pcb);
		pthread_mutex_unlock(&mutexColaBloqueados);


	}
	pthread_mutex_unlock(&mutexListaSemYPCB);
	//free(semYPCB);
}


/*------------------------MEDIANO PLAZO-----------------------------------------*/


/*-------------------PLANIFICACION GENERAL------------------------------------*/



int verificarGradoDeMultiprogramacion(){
	pthread_mutex_lock(&mutex_config_gradoMultiProgramacion);
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	if(gradoMultiProgramacion >= config_gradoMultiProgramacion) {
		pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
		pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);
		return -1;
	}
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
	pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);
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
	log_info(loggerConPantalla, "Pcb encolado en Listos--->PID: %d", procesoListo->pid);
}


void gestionarFinalizacionProgramaEnCpu(int socketCPU){
	t_cpu* cpu;
	_Bool verificaCpu(t_cpu* cpu){
				return (cpu->socket == socketCPU);
			}
			pthread_mutex_lock(&mutexListaCPU);
			cpu = list_remove_by_condition(listaCPU, (void*)verificaCpu);
			cpu->enEjecucion = 2;
			list_add(listaCPU,cpu);
			pthread_mutex_unlock(&mutexListaCPU);

			sem_post(&sem_administrarFinProceso);

}

void liberarRecursosEnMemoria(t_pcb* proceso){
	log_info(loggerConPantalla,"Liberando proceso en memoria--->PID: %d",proceso->pid);
	char comandoFinalizarProceso= 'F';

	send(socketMemoria,&comandoFinalizarProceso,sizeof(char),0);
	send(socketMemoria,&proceso->pid,sizeof(int),0);

}

void aumentarGradoMultiprogramacion(){
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	gradoMultiProgramacion++;
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
}

void disminuirGradoMultiprogramacion(){
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	gradoMultiProgramacion--;
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
}

void listaEsperaATerminados(){
	int indice;
	t_pcb* pcbBuffer;
	t_pcb* aux;

	pthread_mutex_lock(&mutexListaEspera);
	if(!list_is_empty(listaEnEspera)){

		pthread_mutex_lock(&mutexColaTerminados);
		list_add_all(listaEnEspera, colaTerminados);
		pthread_mutex_unlock(&mutexColaTerminados);

		pthread_mutex_lock(&mutexColaEjecucion);
		for (indice = 0; indice < list_size(listaEnEspera); indice++) {

			pcbBuffer = list_remove(listaEnEspera,indice);
			disminuirGradoMultiprogramacion(); // ACA TA

			for (indice = 0; indice < list_size(colaEjecucion)-1; ++indice) {
				aux = list_get(colaEjecucion,indice);
				if(aux->pid == pcbBuffer->pid) list_remove(colaEjecucion, indice);

			}
		}
		pthread_mutex_unlock(&mutexColaEjecucion);
	}
	pthread_mutex_unlock(&mutexListaEspera);
}

int obtenerPaginaSiguiente(int pid){
	int pagina;
	_Bool verificaPid(t_pcb* pcb){
			return pcb->pid == pid;
		}

		_Bool verificaPidContable(t_contable* proceso){
				return proceso->pid == pid;
			}

	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPidContable);

	pthread_mutex_lock(&mutexColaEjecucion);
	t_pcb* pcb = list_remove_by_condition(colaEjecucion,(void*)verificaPid);
	pagina = pcb->cantidadPaginasCodigo + stackSize + proceso->cantPaginasHeap ;
	list_add(colaEjecucion,pcb);
	pthread_mutex_unlock(&mutexColaEjecucion);

	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);


	return pagina;
}


void informarConsola(int socketHiloPrograma,char* mensaje, int size){
	send(socketHiloPrograma,&size,sizeof(int),0);
	send(socketHiloPrograma,mensaje,size,0);
}
#endif /* PLANIFICACION_H_ */
