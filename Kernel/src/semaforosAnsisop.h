/*
 * semaforosAnsisop.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef SEMAFOROSANSISOP_H_
#define SEMAFOROSANSISOP_H_

typedef struct{
	int valor;
	char * id;
}t_semaforo;

typedef struct{
	t_semaforo* semaforo;
	t_list* pids;
}t_semaforoAsociado;

void obtenerSemaforosANSISOPDeLasConfigs();
int tamanioArray(char** array);
t_semaforoAsociado* buscarSemaforo(char* semaforoId);
void waitSemaforoAnsisop(int socketAceptado);
int consultarSemaforo(char* semaforoId);
void disminuirSemaforo(char* semaforoId);
void signalSemaforoAnsisop(int socketAceptado);
void aumentarSemaforo(char* semaforoId);
void chequearColaDeSemaforo(char* semaforoId);
t_list* listaSemaforosAsociados;


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
void waitSemaforoAnsisop(int socketAceptado){
	int tamanio;
	char* semaforo;
	recv(socketAceptado,&tamanio,sizeof(int),0);
	semaforo = malloc(tamanio);
	recv(socketAceptado,semaforo,tamanio,0);
	log_info(loggerConPantalla, "Procesando instruccion atomica Wait--->Semaforo: %s", semaforo);

	int expropiar=consultarSemaforo(semaforo);
	disminuirSemaforo(semaforo);
	send (socketAceptado,&expropiar,sizeof(int),0);

	if(expropiar == -1) {
		/*TODO: Lo que yo digo es lo siguiente. Buscar a la CPU por Socket. SAcarla de la lista de cpus, cambiarle el estado a 3, que representaria qeu hay que expropair por wait*/
		/*Agregar la cpu a la lista devuelta y hacer el signa al semaforo del hilo de mediano plazo.
		 El hilo de mediano plazo, va a pasar ese semaforo, va a sacar a la cpu de la lista de CPUS buscando por estado == 3, va a recibir y deserializar el pcb, y listo.
		 Metes a la cpu devuelta en la cola de cpus, y haces el signal de cpus. Despues con el pcb expropiado, seguis con la logica para Bloqueados.*/
	}
}
void signalSemaforoAnsisop(int socketAceptado){
	int tamanio;
	char* semaforo;
	recv(socketAceptado,&tamanio,sizeof(int),0);
	semaforo = malloc(tamanio);
	recv(socketAceptado,semaforo,tamanio,0);
	log_info(loggerConPantalla, "Procesando instruccion atomica Signal--->Semaforo: %s", semaforo);

	//chequearColaDeSemaforo(semaforo);
	aumentarSemaforo(semaforo);
}

void chequearColaDeSemaforo(char* semaforoId){
	int pid;
	_Bool verificaId(t_semaforoAsociado* semaforoAsociado){
		return !strcmp(semaforoAsociado->semaforo->id,semaforoId);
	}

	t_semaforoAsociado* semaforoAsociado = list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);

	/*TODO: Aca lo mismo. Podes hacer un signal, y hacer la logica de mandar de Bloqueados a Ready, en otro hilo*/
	if(semaforoAsociado->pids->elements_count > 0){ // Tenemos un proceso que esta esperando ese semaforo
		pid = list_remove(semaforoAsociado->pids,0);
		/*Continuara...*/
	}
}

int consultarSemaforo(char* semaforoId){
	int expropiar;

	pthread_mutex_lock(&mutexListaSemaforos);
	t_semaforoAsociado* semaforoAsociado = buscarSemaforo(semaforoId);
	if(semaforoAsociado->semaforo->valor <=0) expropiar = -1;
	else expropiar = 1;
	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);

	return expropiar;
}

void disminuirSemaforo(char* semaforoId){
	pthread_mutex_lock(&mutexListaSemaforos);
	t_semaforoAsociado* semaforoAsociado = buscarSemaforo(semaforoId);
	semaforoAsociado->semaforo->valor -= 1;
	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);

}

void aumentarSemaforo(char* semaforoId){
	pthread_mutex_lock(&mutexListaSemaforos);
	t_semaforoAsociado* semaforoAsociado = buscarSemaforo(semaforoId);
	semaforoAsociado->semaforo->valor += 1;
	list_add(listaSemaforosAsociados,semaforoAsociado);
	pthread_mutex_unlock(&mutexListaSemaforos);
}

t_semaforoAsociado* buscarSemaforo(char* semaforoId){
	_Bool verificaId(t_semaforoAsociado* semaforoAsociado){
				return !strcmp(semaforoAsociado->semaforo->id,semaforoId);
			}

	return list_remove_by_condition(listaSemaforosAsociados,(void*)verificaId);
}

int tamanioArray(char** array){
	int i = 0;
	while(array[i]) i++;
	return i;
}
#endif /* SEMAFOROSANSISOP_H_ */
