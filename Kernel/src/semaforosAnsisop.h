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
	char *id;
}t_semaforo;
/*
typedef struct{
	t_semaforo* semaforo;
	t_list* pids;
}t_semaforoAsociado;
*/
typedef struct{
	char *idSemaforo;
	t_pcb *pcb;

}t_semYPCB;




void obtenerSemaforosANSISOPDeLasConfigs();
void recibirNombreSemaforo(int socketCpu,char ** semaforo);
int tamanioArray(char** array);
void buscarSemaforo(char* semaforoId, t_semaforo **semaforoAsociado);
void waitSemaforoAnsisop(int socketCPU);
int consultarSemaforo(char* semaforoId);
void disminuirSemaforo(char* semaforoId);
void signalSemaforoAnsisop(int socketAceptado);
void aumentarSemaforo(char* semaforoId);
void chequearColaDeSemaforo(char* semaforoId);

t_list* listaSemaforosGlobales;
t_list* listaSemYPCB;

void obtenerSemaforosANSISOPDeLasConfigs(){
	int i;
	int tamanio = tamanioArray(semId);

	/*TODO: Ver los mallocs. Hay char* adnetro de los structs. OJOO*/
	for(i = 0; i < tamanio; i++){

		t_semaforo* semaforo = malloc(sizeof(t_semaforo));
		semaforo->id=semId[i];
		semaforo->valor = strtol(semInit[i], NULL, 10);

		printf("\nSemaforo: %s", semaforo->id);
		printf("\nValor inicial: %d\n", semaforo->valor);

		/*t_semaforoAsociado* semaforoAsociado = malloc(sizeof(t_semaforo));
		semaforoAsociado->pids=list_create();
		semaforoAsociado->semaforo = semaforo;*/
		list_add(listaSemaforosGlobales,semaforo/*Asociado*/);
		}
}

void recibirNombreSemaforo(int socketCpu,char ** semaforo){
	int tamanio;
	recv(socketCpu,&tamanio,sizeof(int),0);
	*semaforo = malloc(tamanio);
	recv(socketCpu,*semaforo,tamanio,0);
	strcpy(*semaforo + tamanio,"\0");
}

void waitSemaforoAnsisop(int socketCPU){
	int expropiar;
	char* semaforo;
	t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));

	recibirNombreSemaforo(socketCPU,&semaforo);
	log_info(loggerConPantalla, "Procesando instruccion atomica Wait--/*->semaforo*/: %s", semaforo);

	if((expropiar = consultarSemaforo(semaforo)) > 0) disminuirSemaforo(semaforo);//ESTO ESTA BIEN ASI
	send(socketCPU,&expropiar,sizeof(int),0);

	if(expropiar == -1) {

		semYPCB->pcb = recibirYDeserializarPcb(socketCPU);

		pthread_mutex_lock(&mutexColaBloqueados);
		cpuEjecucionAOciosa(socketCPU);
		pthread_mutex_unlock(&mutexColaBloqueados);

		semYPCB->idSemaforo = semaforo;

		printf("\n\nSEMAFORO ID: %s\n\n", semYPCB->idSemaforo);
		printf("\n\nSEMAFORO ID: %d\n\n", semYPCB->pcb->pid);

		pthread_mutex_lock(&mutexListaSemYPCB);
		list_add(listaSemYPCB, semYPCB);
		pthread_mutex_unlock(&mutexListaSemYPCB);

		sem_post(&sem_ListaSemYPCB);
	}
}
void signalSemaforoAnsisop(int socketAceptado){
	char* semaforo;

	recibirNombreSemaforo(socketAceptado,&semaforo);
	aumentarSemaforo(semaforo);

	log_info(loggerConPantalla, "Procesando instruccion atomica Signal--/*->semaforo*/: %s", semaforo);


	sem_post(&sem_semAumentados);








	/*pthread_mutex_lock(&mutexListaSemAumentados);
	if(semAumentado->semaforo->valor > 0) list_add(listaSemAumentados, (void*)semAumentado->semaforo->id);
	pthread_mutex_unlock(&mutexListaSemAumentados);

	sem_post(&sem_semAumentados);*/
	//free(semAumentado);
	//chequearColaDeSemaforo(semaforo); NO LO USO
}

/*void chequearColaDeSemaforo(char* semaforoId){
	int pid;
	_Bool verificaId(t_semaforoAsociado* semaforoAsociado){
		return !strcmp(semaforoAsociado->semaforo->id,semaforoId);
	}

	t_semaforoAsociado* semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);

	/*TODO: Aca lo mismo. Podes hacer un signal, y hacer la logica de mandar de Bloqueados a Ready, en otro hilo
	if(semaforoAsociado->pids->elements_count > 0){ // Tenemos un proceso que esta esperando ese semaforo
		pid = list_remove(semaforoAsociado->pids,0);
		/*Continuara...*/
	/*}
}*/

int consultarSemaforo(char* semaforoId){
	int expropiar = 1;
	t_semaforo* semaforoAsociado = malloc(sizeof(t_semaforo));

	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo/*->semaforo*/->id,semaforoId));
	}

	pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);

	printf("\n\nSemaforo id: %s", semaforoAsociado/*->semaforo*/->id);
	printf("\nSemaforo valor: %d\n\n", semaforoAsociado/*->semaforo*/->valor);

	if(semaforoAsociado/*->semaforo*/->valor < 1) expropiar = -1;

	list_add(listaSemaforosGlobales,semaforoAsociado);

	pthread_mutex_unlock(&mutexListaSemaforos);

/*
	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	if(semaforoAsociado/*->semaforo->valor < 1) expropiar = -1;

	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);
*/
	//free(semaforoAsociado);

	return expropiar;
}

void disminuirSemaforo(char* semaforoId){


	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo/*->semaforo*/->id,semaforoId));
	}

	t_semaforo *semaforoAsociado;
	pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);
	semaforoAsociado/*->semaforo*/->valor -= 1;
	list_add(listaSemaforosGlobales,semaforoAsociado);

	pthread_mutex_unlock(&mutexListaSemaforos);

	/*
	t_semaforoAsociado *semaforoAsociado = malloc(sizeof(t_semaforoAsociado));
	pthread_mutex_lock(&mutexListaSemaforos);
	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	semaforoAsociado/*->semaforo->valor -= 1;
	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);
*/
}

void aumentarSemaforo(char* semaforoId){ /*TODO: Ojo con lo que se haga con el semaforo devuelto. Hay que eliminar la entrada anterior y actualizar por la nueva*/

	_Bool verificaId(t_semaforo* semaforo){
			return (!strcmp(semaforo/*->semaforo*/->id,semaforoId));
	}

	t_semaforo *semaforoAsociado;
	pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);
	semaforoAsociado/*->semaforo*/->valor += 1;

	list_add(listaSemaforosGlobales,semaforoAsociado);

	pthread_mutex_unlock(&mutexListaSemaforos);

	/*
	pthread_mutex_lock(&mutexListaSemaforos);
	semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
	semaforoAsociado/*->semaforo->valor += 1;
	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);
	*/

}

void buscarSemaforo(char* semaforoId, t_semaforo **semaforoAsociado){

	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo/*->semaforo*/->id,semaforoId));
		}

	if(list_any_satisfy(listaSemaforosGlobales, (void*) verificaId)){
		*semaforoAsociado = list_remove_by_condition(listaSemaforosGlobales,(void*)verificaId);
	}
}


int tamanioArray(char** array){
	int i = 0;
	while(array[i]) i++;
	return i;
}
#endif /* SEMAFOROSANSISOP_H_ */
