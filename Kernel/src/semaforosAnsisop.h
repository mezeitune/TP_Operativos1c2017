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

		list_add(listaSemaforosGlobales,semaforo);
		}
}

void recibirNombreSemaforo(int socketCpu,char ** semaforo){
	int tamanio;
	recv(socketCpu,&tamanio,sizeof(int),0);

	*semaforo = malloc(tamanio + sizeof(char));
	recv(socketCpu,*semaforo,tamanio,0);
	strcpy(*semaforo + tamanio,"\0");
}

void waitSemaforoAnsisop(int socketCPU){
	int expropiar,pid;
	char* semaforo;
	int rafagasEjecutadas;
	char ok = 'O';
	t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));

	recv(socketCPU,&pid,sizeof(int),0);

	recibirNombreSemaforo(socketCPU,&semaforo);
	log_info(loggerConPantalla, "Procesando instruccion atomica Wait---> %s", semaforo);


	if((expropiar = consultarSemaforo(semaforo)) > 0)disminuirSemaforo(semaforo);
	send(socketCPU,&expropiar,sizeof(int),0);

	if(expropiar == -1) {

		recv(socketCPU, &rafagasEjecutadas, sizeof(int), 0);
		semYPCB->pcb = recibirYDeserializarPcb(socketCPU);


		actualizarRafagas(semYPCB->pcb->pid,rafagasEjecutadas);

		cpuEjecucionAFQPB(socketCPU);
		//send(socketCPU, &ok,sizeof(char),0);

		semYPCB->idSemaforo = semaforo;

		pthread_mutex_lock(&mutexListaSemYPCB);
		list_add(listaSemYPCB, semYPCB);
		pthread_mutex_unlock(&mutexListaSemYPCB);

		sem_post(&sem_ListaSemYPCB);
	}
	actualizarSysCalls(pid);
	return;
}
void signalSemaforoAnsisop(int socketCpu){
	char* semaforo;
	int pid;
	recv(socketCpu,&pid,sizeof(int),0);

	recibirNombreSemaforo(socketCpu,&semaforo);
	aumentarSemaforo(semaforo);

	log_info(loggerConPantalla, "Procesando instruccion atomica Signal--->: %s", semaforo);

	actualizarSysCalls(pid);
	sem_post(&sem_semAumentados);

	return;
}
int consultarSemaforo(char* semaforoId){
	int expropiar = 1;
	t_semaforo* semaforoAsociado = malloc(sizeof(t_semaforo));

	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo->id,semaforoId));
	}

	pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);
	list_add(listaSemaforosGlobales,semaforoAsociado);

	pthread_mutex_unlock(&mutexListaSemaforos);

	log_info(loggerConPantalla,"Semaforo id: %s", semaforoAsociado->id);
	log_info(loggerConPantalla,"\nSemaforo valor: %d", semaforoAsociado->valor);

	if(semaforoAsociado->valor < 1) expropiar = -1;

	return expropiar;
}

void disminuirSemaforo(char* semaforoId){


	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo->id,semaforoId));
	}

	t_semaforo *semaforoAsociado;



	pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);


	log_info(loggerConPantalla,"Semaforo id: %s", semaforoAsociado->id);
	log_info(loggerConPantalla,"\nSemaforo valor: %d", semaforoAsociado->valor);

	semaforoAsociado->valor -= 1;
	list_add(listaSemaforosGlobales,semaforoAsociado);


	log_info(loggerConPantalla,"Semaforo id: %s", semaforoAsociado->id);
	log_info(loggerConPantalla,"\nSemaforo valor disminuido: %d", semaforoAsociado->valor);

	pthread_mutex_unlock(&mutexListaSemaforos);

}

void aumentarSemaforo(char* semaforoId){ /*TODO: Ojo con lo que se haga con el semaforo devuelto. Hay que eliminar la entrada anterior y actualizar por la nueva*/

	_Bool verificaId(t_semaforo* semaforo){
			return (!strcmp(semaforo->id,semaforoId));
	}

	t_semaforo *semaforoAsociado;
	pthread_mutex_lock(&mutexListaSemaforos);



	buscarSemaforo(semaforoId,&semaforoAsociado);

	log_info(loggerConPantalla,"Semaforo id: %s", semaforoAsociado->id);
	log_info(loggerConPantalla,"\nSemaforo valor: %d", semaforoAsociado->valor);


	semaforoAsociado->valor += 1;
	list_add(listaSemaforosGlobales,semaforoAsociado);



	log_info(loggerConPantalla,"Semaforo id: %s", semaforoAsociado->id);
	log_info(loggerConPantalla,"\nSemaforo valor aumentado: %d", semaforoAsociado->valor);


	pthread_mutex_unlock(&mutexListaSemaforos);



}

void buscarSemaforo(char* semaforoId, t_semaforo **semaforoAsociado){

	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo->id,semaforoId));
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
