/*
 * semaforosAnsisop.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef SEMAFOROSANSISOP_H_
#define SEMAFOROSANSISOP_H_

#include "comandosCPU.h"



typedef struct{
	int valor;
	char * id;
}t_semaforo;

typedef struct{
	t_semaforo* semaforo;
	t_list* pids;
}t_semaforoAsociado;

typedef struct{
	char *idSemaforo;
	t_pcb *pcb;

}t_semYPCB;




void obtenerSemaforosANSISOPDeLasConfigs();
int tamanioArray(char** array);
void buscarSemaforo(char* semaforoId, t_semaforoAsociado **semaforoAsociado);
void waitSemaforoAnsisop(int socketCPU);
int consultarSemaforo(char* semaforoId);
void disminuirSemaforo(char* semaforoId);
void signalSemaforoAnsisop(int socketAceptado);
t_semaforoAsociado *aumentarSemaforo(char* semaforoId);
void chequearColaDeSemaforo(char* semaforoId);

t_list* listaSemaforosAsociados;
t_list* listaProcesosBloqueados;
t_list* listaSemAumentados;

void obtenerSemaforosANSISOPDeLasConfigs(){
	int i;
	int tamanio = tamanioArray(semId);

	/*TODO: Ver los mallocs. Hay char* adnetro de los structs. OJOO*/
	for(i = 0; i < tamanio; i++){

		t_semaforo* semaforo = malloc(sizeof(t_semaforo));
		semaforo->id=semId[i];
		semaforo->valor = strtol(semInit[i], NULL, 10);

		t_semaforoAsociado* semaforoAsociado = malloc(sizeof(t_semaforoAsociado));
		semaforoAsociado->pids=list_create();
		semaforoAsociado->semaforo = semaforo;
		list_add(listaSemaforosAsociados,semaforoAsociado);
		}
}
void waitSemaforoAnsisop(int socketCPU){
	int tamanio;
	int expropiar;
	char semaforo;

	t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));


	recv(socketCPU,&tamanio,sizeof(int),0);

	printf("\n\ntamanio: %d\n\n", tamanio);

	//semaforo = malloc(tamanio);

	recv(socketCPU,&semaforo,tamanio,0);

	log_info(loggerConPantalla, "Procesando instruccion atomica Wait--->Semaforo: %c", semaforo);
	printf("\n\nSEMAFORO: %c\n\n", semaforo);


	expropiar = consultarSemaforo(&semaforo);
	disminuirSemaforo(&semaforo);

	send(socketCPU,&expropiar,sizeof(int),0);


	if(expropiar == -1) {

		semYPCB->pcb = recibirYDeserializarPcb(socketCPU);

		printf("\n\nPCB:%d\n\n", semYPCB->pcb->pid);

		cpuEjecucionAOciosa(socketCPU);

		semYPCB->idSemaforo = &semaforo;


		pthread_mutex_lock(&mutexListaProcesosBloqueados);
		list_add(listaProcesosBloqueados, semYPCB);
		pthread_mutex_unlock(&mutexListaProcesosBloqueados);

		sem_post(&sem_ListaProcesosBloqueados);

	}
}
void signalSemaforoAnsisop(int socketAceptado){
	int tamanio;
	char* semaforo;

	t_semaforoAsociado *semAumentado;

	recv(socketAceptado,&tamanio,sizeof(int),0);
	semaforo = malloc(tamanio);
	recv(socketAceptado,semaforo,tamanio,0);

	semAumentado = aumentarSemaforo(semaforo);

	log_info(loggerConPantalla, "Procesando instruccion atomica Signal--->Semaforo: %s", semaforo);
	printf("\n\nSemaforo Aumentado: %s\n\n", semAumentado->semaforo->id);


	pthread_mutex_lock(&mutexListaSemAumentados);
	if(semAumentado->semaforo->valor > 0) list_add(listaSemAumentados, semAumentado->semaforo->id);
	pthread_mutex_unlock(&mutexListaSemAumentados);

	sem_post(&sem_listaSemAumentados);
	//free(semAumentado);
	//chequearColaDeSemaforo(semaforo);
}

/*void chequearColaDeSemaforo(char* semaforoId){
	int pid;
	_Bool verificaId(t_semaforoAsociado* semaforoAsociado){
		return !strcmp(semaforoAsociado->semaforo->id,semaforoId);
	}

	t_semaforoAsociado* semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);

	/*TODO: Aca lo mismo. Podes hacer un signal, y hacer la logica de mandar de Bloqueados a Ready, en otro hilo*/
	/*if(semaforoAsociado->pids->elements_count > 0){ // Tenemos un proceso que esta esperando ese semaforo
		pid = list_remove(semaforoAsociado->pids,0);
		/*Continuara...*/
	/*}
}*/

int consultarSemaforo(char* semaforoId){
	int expropiar;
	t_semaforoAsociado* semaforoAsociado = malloc(sizeof(t_semaforo));

	_Bool verificaId(t_semaforoAsociado* semaforo){
				return (!strcmp(semaforo->semaforo->id,semaforoId));
	}

	pthread_mutex_lock(&mutexListaSemaforos);
	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);

	if(semaforoAsociado->semaforo->valor <= 0) expropiar = -1;
	else expropiar = 1;

	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);

	//free(semaforoAsociado);

	return expropiar;
}

void disminuirSemaforo(char* semaforoId){


	_Bool verificaId(t_semaforoAsociado* semaforo){
				return (!strcmp(semaforo->semaforo->id,semaforoId));
	}


	printf("\n\nSemaforo a Disminuir: %s\n\n", semaforoId);

	t_semaforoAsociado *semaforoAsociado = malloc(sizeof(t_semaforoAsociado));


	pthread_mutex_lock(&mutexListaSemaforos);

	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	semaforoAsociado->semaforo->valor -= 1;


	list_add(listaSemaforosAsociados,semaforoAsociado);
	printf("\n\nSemaforo a Disminuir: %s\n\n", semaforoAsociado->semaforo->id);
	printf("\n\nValor Semaforo a Disminuir: %d\n\n", semaforoAsociado->semaforo->valor);

	pthread_mutex_unlock(&mutexListaSemaforos);

	//free(semaforoAsociado);

}

t_semaforoAsociado *aumentarSemaforo(char* semaforoId){

	_Bool verificaId(t_semaforoAsociado* semaforo){
			return (!strcmp(semaforo->semaforo->id,semaforoId));
	}

	t_semaforoAsociado *semaforoAsociado;


	pthread_mutex_lock(&mutexListaSemaforos);
	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	semaforoAsociado->semaforo->valor += 1;

	printf("\n\nSemaforo a Aumentar: %s\n\n", semaforoAsociado->semaforo->id);
	printf("\n\nValor Semaforo a Aumentar: %d\n\n", semaforoAsociado->semaforo->valor);

	list_add(listaSemaforosAsociados,semaforoAsociado);

	pthread_mutex_unlock(&mutexListaSemaforos);

	return semaforoAsociado;

}

/*void buscarSemaforo(char* semaforoId, t_semaforoAsociado **semaforoAsociado){


		return (!strcmp(semaforo->semaforo->id,semaforoId));
	}

	if(list_any_satisfy(listaSemaforosAsociados, (void*) verificaId)){
		printf("\n\nSEM ID: %s\n\n", semaforoId);
		printf("\n\nTAMANIO LISTA SEMAFOROS: %d\n\n", list_size(listaSemaforosAsociados));
		*semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	}
	else printf("\n\nNO HAY NADA\n\n");
}*/

int tamanioArray(char** array){
	int i = 0;
	while(array[i]) i++;
	return i;
}
#endif /* SEMAFOROSANSISOP_H_ */
