/*
 * contabilidad.h
 *
 *  Created on: 8/6/2017
 *      Author: utnso
 */

#ifndef CONTABILIDAD_H_
#define CONTABILIDAD_H_

#include  "pcb.h"
#include "sincronizacion.h"

typedef struct{
	int pid;
	int cantRafagas;
	//int cantOpPrivilegiadas;
	int cantPaginasHeap;
	//lo del heap
	int cantSysCalls;
}t_contable;


t_list* listaContable;
void crearInformacionContable(int pid);
void actualizarRafagas(int pid, int rafagas);


void crearInformacionContable(int pid){
	t_contable* contabilidadProceso = malloc(sizeof(t_contable));
	contabilidadProceso->pid=pid;
	contabilidadProceso->cantRafagas=0;
	contabilidadProceso->cantSysCalls=0;
	contabilidadProceso->cantPaginasHeap=0;

	pthread_mutex_lock(&mutexListaContable);
	list_add(listaContable,contabilidadProceso);
	pthread_mutex_unlock(&mutexListaContable);

}

void actualizarPaginasHeap(int pid){
	_Bool verificaPid(t_contable* contabilidad){
		return contabilidad->pid == pid;
	}
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = list_remove_by_condition(listaContable,(void*)verificaPid);
	contabilidad->cantPaginasHeap+=1;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void actualizarRafagas(int pid, int rafagas){
	_Bool verificaPid(t_contable* contabilidad){
		return contabilidad->pid == pid;
	}
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = list_remove_by_condition(listaContable,(void*)verificaPid);
	contabilidad->cantRafagas += rafagas;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);

}

#endif /* CONTABILIDAD_H_ */
