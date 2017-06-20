/*

 * heap.h
 *
 *  Created on: 19/6/2017
 *      Author: utnso
 */

#ifndef HEAP_H_
#define HEAP_H_
#include "configuraciones.h"
#include <commons/collections/list.h>
#include "conexionMemoria.h"

typedef struct
{
	int pagina;
	int pid;
	int sizeDisponible;
}t_adminBloqueMetadata;

typedef struct
{
	int bitUso;
	int size;
}__attribute__((packed)) t_bloqueMetadata;

typedef struct{
	int pagina;
	int offset;
}t_punteroCpu;

t_list* listaAdmHeap;

void reservarEspacioHeap(int pid, int size,int socket);
int verificarEspacioLibreHeap(int size, int pid);
void reservarPaginaHeap(int pid);
void compactarPaginaHeap(int pagina, int pid);
void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido);
void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido);
int reservarBloqueHeap(int pid,int size,int pagina);
void destruirPaginaHeap(int pidProc, int pagina);
void destruirTodasLasPaginasHeapDeProceso(int pidProc);

void reservarEspacioHeap(int pid, int size, int socket){
	log_info(loggerConPantalla,"Reservando espacio de memoria dinamica");
	t_punteroCpu* puntero = malloc(sizeof(t_punteroCpu));

	puntero->pagina = verificarEspacioLibreHeap(size, pid);
	if(puntero->pagina  == -1){
		log_error(loggerConPantalla,"No hay espacio suficiente en memoria para reservar una nueva pagina");
		/*TODO: Avisar a Consola, expropiar proceso y terminarlo, liberando recursos*/
			return;
		}
	reservarPaginaHeap(pid);
	puntero->offset = reservarBloqueHeap(pid, size, puntero->pagina);

	send(socket,&puntero->pagina,sizeof(int),0);
	send(socket,&puntero->offset,sizeof(int),0);
}


int verificarEspacioLibreHeap(int size, int pid){
	log_info(loggerConPantalla,"Verificando espacio libre para memoria dinamica");
	int i = 0;
	t_adminBloqueMetadata* aux;
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->sizeDisponible >= size && aux->pid == pid)
		{
			compactarPaginaHeap(aux->pagina,aux->pid);
			return aux->pagina;
		}
	}
	return -1;
}


void reservarPaginaHeap(int pid){ //Reservo una página de heap nueva para el proceso
	log_info(loggerConPantalla,"Reservando pagina de heap");
	t_bloqueMetadata* aux = malloc(sizeof(t_bloqueMetadata));
	void* buffer=malloc(sizeof(t_bloqueMetadata));
	aux->size = config_paginaSize - sizeof(t_bloqueMetadata);
	aux->bitUso = -1;

	memcpy(buffer,&aux->bitUso,sizeof(int));
	memcpy(buffer + sizeof(int),&aux->size,sizeof(int));

	int paginaReservada = reservarPaginaEnMemoria(pid);
	escribirEnMemoria(paginaReservada,pid,0,aux->size,buffer);  //Para indicar que está sin usar y que tiene tantos bits libres para utilizarse

	free(aux);
	free(buffer);

	t_adminBloqueMetadata* bloqueAdmin= malloc(sizeof(t_adminBloqueMetadata));
	bloqueAdmin->pagina = paginaReservada;
	bloqueAdmin->pid = pid;
	bloqueAdmin->sizeDisponible = aux->size;
	list_add(listaAdmHeap, bloqueAdmin);
}


void compactarPaginaHeap(int pagina, int pid){
	log_info(loggerConPantalla,"Compactando pagina de heap");
	int offset = 0;
	int desplazamiento=0;
	t_bloqueMetadata* actual = malloc(sizeof(t_bloqueMetadata));
	t_bloqueMetadata* siguiente = malloc(sizeof(t_bloqueMetadata));
	void* buffer= malloc(sizeof(t_bloqueMetadata));

	while(offset < config_paginaSize){
		buffer = leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata));
		actual->bitUso = *(int*)(buffer+desplazamiento);
		desplazamiento += sizeof(int);
		actual->size= *(int*)(buffer+desplazamiento);
		desplazamiento = 0;


		buffer = leerDeMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata)+ actual->size ,sizeof(t_bloqueMetadata));
		siguiente->bitUso = *(int*)(buffer+desplazamiento);
		desplazamiento += sizeof(int);
		siguiente->size= *(int*)(buffer+desplazamiento);

		if(actual->bitUso == -1 && siguiente->bitUso == -1){
			actual->size = actual->size + sizeof(t_bloqueMetadata) + siguiente->size;
		}
		else{
			offset += sizeof(t_bloqueMetadata) + actual->size;
		}
	}
	free(buffer);
}

void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido){
	escribirEnMemoria(pagina,pid,offset+sizeof(t_bloqueMetadata),size,contenido);
}

void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido){
	*contenido = leerDeMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size);
}

int reservarBloqueHeap(int pid,int size,int pagina){
	log_info(loggerConPantalla,"Reservando bloque de memoria dinamica en heap");
	t_bloqueMetadata* auxBloque = malloc(sizeof(t_bloqueMetadata));
	t_adminBloqueMetadata* aux = malloc(sizeof(t_adminBloqueMetadata));
	int offset;
	int i = 0;
	int sizeReal = size;
	void* buffer;
	int desplazamiento=0;
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pid)
		{
			if(aux->sizeDisponible <= size + sizeof(t_bloqueMetadata)){
				sizeReal = aux->sizeDisponible;
				aux->sizeDisponible = 0;
			}
			else{
				aux->sizeDisponible = aux->sizeDisponible - size;
			}
			list_replace(listaAdmHeap,i,aux);
			break;
		}
	}

	while(i < config_paginaSize){
		auxBloque = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		if(auxBloque->size >= size){
			buffer=malloc(sizeof(t_bloqueMetadata));
			auxBloque->bitUso = 1;
			auxBloque->size = sizeReal;

			memcpy(buffer,&auxBloque->bitUso,sizeof(int));
			desplazamiento += sizeof(int);
			memcpy(buffer + desplazamiento, &auxBloque->size,sizeof(int));
			desplazamiento = 0;

			escribirEnMemoria(pid,pagina,i,sizeof(t_bloqueMetadata),buffer); //Escribo y reservo el metadata que se quiere reservar
			free(buffer);
			offset = i;
			if(aux->sizeDisponible > 0){
				buffer=malloc(sizeof(t_bloqueMetadata));
				auxBloque->bitUso = -1;
				auxBloque->size = aux->sizeDisponible;

				memcpy(buffer,&auxBloque->bitUso,sizeof(int));
				desplazamiento += sizeof(int);
				memcpy(buffer + desplazamiento, &auxBloque->size,sizeof(int));
				desplazamiento = 0;

				escribirEnMemoria(pid,pagina,i+sizeof(t_bloqueMetadata)+sizeReal,sizeof(t_bloqueMetadata),buffer); //ANuncio cuanto espacio libre queda en el heap en el siguiente metadata
				free(buffer);
			}
		break;
		}

	}
	return offset;
}

void destruirPaginaHeap(int pidProc, int pagina){ //Si quiero destruir una página específica de la lista
	t_adminBloqueMetadata* aux;
	int i = 0;

	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
			break;
		}
	}
}

void destruirTodasLasPaginasHeapDeProceso(int pidProc){ //Elimino todas las estructuras administrativas de heap asociadas a un PID
	t_adminBloqueMetadata* aux;
	int i = 0;

	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
		}
	}
}

#endif /* HEAP_H_ */
