/*
 * sincronizacion.h

 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef SINCRONIZACION_H_
#define SINCRONIZACION_H_

#include <pthread.h>
#include <semaphore.h>

void inicializarSemaforos();
pthread_mutex_t mutexColaNuevos;
pthread_mutex_t mutexColaListos;
pthread_mutex_t mutexColaTerminados;
pthread_mutex_t mutexListaConsolas;
pthread_mutex_t mutexListaCPU;
pthread_mutex_t mutexConexion;
pthread_mutex_t mutexGradoMultiProgramacion;
sem_t sem_admitirNuevoProceso;
sem_t sem_colaReady;
sem_t sem_CPU;


void inicializarSemaforos(){
		pthread_mutex_init(&mutexColaNuevos,NULL);
		pthread_mutex_init(&mutexColaListos, NULL);
		pthread_mutex_init(&mutexColaTerminados, NULL);
		pthread_mutex_init(&mutexListaConsolas,NULL);
		pthread_mutex_init(&mutexListaCPU,NULL);
		pthread_mutex_init(&mutexGradoMultiProgramacion,NULL);
		pthread_mutex_init(&mutexConexion,NULL);
		sem_init(&sem_admitirNuevoProceso, 0, 0);
		sem_init(&sem_colaReady,0,0);
		sem_init(&sem_CPU,0,0);

}

#endif /* SINCRONIZACION_H_ */
