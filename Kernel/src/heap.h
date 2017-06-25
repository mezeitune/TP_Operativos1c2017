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
#include "contabilidad.h"

typedef struct
{
	int pagina;
	int pid;
	int sizeDisponible;
}t_adminBloqueHeap;


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
int reservarPaginaHeap(int pid,int pagina);
void compactarPaginaHeap(int pagina, int pid);
void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido);
void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido);
int reservarBloqueHeap(int pid,int size,int pagina);
void destruirPaginaHeap(int pidProc, int pagina);
void destruirTodasLasPaginasHeapDeProceso(int pidProc);
int paginaHeapBloqueSuficiente(int posicionPaginaHeap,int pagina,int pid,int size);
void liberarBloqueHeap(int pid, int pagina, int offset);

void reservarEspacioHeap(int pid, int size, int socket){
	log_info(loggerConPantalla,"---Reservando espacio de memoria dinamica---\n");
	t_punteroCpu* puntero = malloc(sizeof(t_punteroCpu));


	puntero->pagina = verificarEspacioLibreHeap(size, pid);

	if(puntero->pagina  == -1){

		puntero->pagina = obtenerPaginaSiguiente(pid);
		if(reservarPaginaHeap(pid,puntero->pagina)<0){
			log_error(loggerConPantalla,"No hay espacio suficiente en memoria para reservar una nueva pagina\n");
			/*TODO: Avisar a Consola, expropiar proceso y terminarlo, liberando recursos*/
				return;
			}
		aumentarPaginasHeap(pid);
		}


	puntero->offset = reservarBloqueHeap(pid, size, puntero->pagina);

	printf("\nPagina que se le da para ese espacio de memoria:%d\n",puntero->pagina);
	send(socket,&puntero->pagina,sizeof(int),0);
	send(socket,&puntero->offset,sizeof(int),0);
}



int verificarEspacioLibreHeap(int size, int pid){
	log_info(loggerConPantalla,"----Verificando espacio libre para memoria dinamica-----\n");
	int i = 0;
	printf("PID Del Proceso:%d\n",pid);
	printf("Size A Reservar:%d\n",size);
	t_adminBloqueHeap* aux;
	printf("Lecturas:\n");
	while(i < list_size(listaAdmHeap))
	{
		aux = (t_adminBloqueHeap*) list_get(listaAdmHeap,i);
		printf("i=%d\n",i);
		printf("sizeDisponible=%d\n",aux->sizeDisponible);
		printf("pid=%d\n",aux->pid);
		if(aux->sizeDisponible >= size && aux->pid == pid)
		{
			compactarPaginaHeap(aux->pagina,aux->pid);
			if(paginaHeapBloqueSuficiente(i,aux->pagina,aux->pid,size) > 0){
				return aux->pagina;
			}
		}
		i++;
	}
	return -1;
}


int reservarPaginaHeap(int pid,int pagina){ //Reservo una página de heap nueva para el proceso
	log_info(loggerConPantalla,"Reservando pagina de heap--->PID:%d",pid);

	t_bloqueMetadata aux ;

	void* buffer=malloc(sizeof(t_bloqueMetadata));
	aux.bitUso = -1;
	aux.size = config_paginaSize - sizeof(t_bloqueMetadata);
	memcpy(buffer,&aux,sizeof(t_bloqueMetadata));

	reservarPaginaEnMemoria(pid);

	int resultadoEjecucion=escribirEnMemoria(pid,pagina,0,sizeof(t_bloqueMetadata),buffer);  //Para indicar que está sin usar y que tiene tantos bits libres para utilizarse


	t_adminBloqueHeap* bloqueAdmin=malloc(sizeof(t_adminBloqueHeap));
	bloqueAdmin->pagina = pagina;
	bloqueAdmin->pid = pid;
	bloqueAdmin->sizeDisponible = aux.size;
	printf("Pagina Reservada:%d\n",bloqueAdmin->pagina);
	printf("PID Proceso Reservado:%d\n",bloqueAdmin->pid);
	printf("Size Disponible Pagina Reservada:%d\n",bloqueAdmin->sizeDisponible);
	list_add(listaAdmHeap, bloqueAdmin);

	free(buffer);
	return resultadoEjecucion;
}


void compactarPaginaHeap(int pagina, int pid){
	log_info(loggerConPantalla,"Compactando pagina de heap");
	int offset = 0;
	t_bloqueMetadata actual;
	t_bloqueMetadata siguiente;
	t_bloqueMetadata* buffer= malloc(sizeof(t_bloqueMetadata));

	actual.size = 0;

	while(offset < config_paginaSize && offset + sizeof(t_bloqueMetadata) + actual.size > config_paginaSize - sizeof(t_bloqueMetadata)){
		buffer = (t_bloqueMetadata*) leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata)); //Leo el metadata Actual
		//memcpy(&actual->bitUso,buffer, sizeof(int));
		//memcpy(&actual->size,buffer + sizeof(int) , sizeof(int));
		actual.bitUso = buffer->bitUso;
		actual.size = buffer ->size;

		/*if(offset + sizeof(t_bloqueMetadata) + actual.size > config_paginaSize - sizeof(t_bloqueMetadata)){ //Me fijo que el metadata siguiente esté dentro de esta página
			break;
		}*/

		buffer = (t_bloqueMetadata*) leerDeMemoria(pid,pagina,offset + sizeof(t_bloqueMetadata) + actual.size,sizeof(t_bloqueMetadata)); //Leo la posición del metadata que le sigue al actual
		//memcpy(&siguiente->bitUso,buffer , sizeof(int));
		//memcpy(&siguiente->size,buffer + sizeof(int) , sizeof(int));

		siguiente.bitUso = buffer->bitUso;
		siguiente.size = buffer ->size;

		printf("Actual bitUso=%d\n",actual.bitUso);
		printf("Actual size=%d\n",actual.size);
		printf("Siguiente bitUso=%d\n",siguiente.bitUso);
		printf("Siguiente size=%d\n",siguiente.size);

		if(actual.bitUso == -1 && siguiente.bitUso == -1){

			actual.size = actual.size + sizeof(t_bloqueMetadata) + siguiente.size;
			memcpy(buffer,&actual,sizeof(t_bloqueMetadata));

			escribirEnMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata),(void *) buffer); //Actualizo el metadata en el que me encuentro parado en la memoria

		}
		else{
			offset += sizeof(t_bloqueMetadata) + actual.size;
		}
	}
	free(buffer);
	printf("Saliendo de compactar pagina heap\n");
}

void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido){
	escribirEnMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size,contenido);
}

void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido){
	*contenido = leerDeMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size);
}

int reservarBloqueHeap(int pid,int size,int pagina){
	log_info(loggerConPantalla,"--Reservando bloque de memoria dinamica en heap de pagina:%d--\n",pagina);
	t_bloqueMetadata auxBloque;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));
	int offset=0;
	int i = 0;
	int sizeReal = size;
	void *buffer=malloc(sizeof(t_bloqueMetadata));

	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pid){
			if(aux->sizeDisponible <= size + sizeof(t_bloqueMetadata)){
				sizeReal = aux->sizeDisponible;
				aux->sizeDisponible = 0;
			}
			else{
				aux->sizeDisponible = aux->sizeDisponible - size - sizeof(t_bloqueMetadata);
			}
			list_replace(listaAdmHeap,i,aux);
			break;
		}
		i++;
	}
	i = 0;
	while(i < config_paginaSize){


		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

		//memcpy(&auxBloque->bitUso,buffer , sizeof(int));
		//memcpy(&auxBloque->size,buffer + sizeof(int), sizeof(int));

		printf("Leo:\n");
		printf("Pagina:%d\n",pagina);
		printf("Offset:%d\n",i);
		printf("BitUso:%d\n",auxBloque.bitUso);
		printf("Size:%d\n",auxBloque.size);

		if(auxBloque.size >= size && auxBloque.bitUso == -1){

			auxBloque.bitUso = 1;
			auxBloque.size = sizeReal;
			memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));
			printf("Escribo:\n");
			printf("BitUso:%d\n",auxBloque.bitUso);
			printf("Size:%d\n",auxBloque.size);
			printf("Pagina:%d\n",pagina);
			printf("Offset:%d\n",i);

			escribirEnMemoria(pid,pagina,i,sizeof(t_bloqueMetadata),buffer); //Escribo y reservo el metadata que se quiere reservar

			offset = i;
			if(aux->sizeDisponible > 0){
				auxBloque.bitUso = -1;
				auxBloque.size = aux->sizeDisponible;

				printf("Escribo:\n");
				printf("Bit uso: %d\n",auxBloque.bitUso);
				printf("Size disponible: %d\n",auxBloque.size);
				printf("Pagina:%d\n",pagina);
				printf("Offset:%d",i);
				memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));

				escribirEnMemoria(pid,pagina,i+sizeof(t_bloqueMetadata)+sizeReal,sizeof(t_bloqueMetadata),buffer); //Anuncio cuanto espacio libre queda en el heap en el siguiente metadata

			}
			break;
		}
		else{
			i = i + sizeof(t_bloqueMetadata) + auxBloque.size;
		}

	}
	printf("Terminada reserva bloque heap\n");
	free(buffer);
	//free(auxBloque);
	//free(aux);
	return offset;
}

void destruirPaginaHeap(int pidProc, int pagina){ //Si quiero destruir una página específica de la lista
	t_adminBloqueHeap* aux;
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
	t_adminBloqueHeap* aux;
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

int paginaHeapBloqueSuficiente(int posicionPaginaHeap,int pagina,int pid ,int size){
	printf("Pagina Heap Bloque Suficiente\n");
	int i = 0;

	t_adminBloqueHeap* aux;
	aux = (t_adminBloqueHeap*) list_get(listaAdmHeap,posicionPaginaHeap);

	t_bloqueMetadata auxBloque;
	void *buffer= malloc(sizeof(t_bloqueMetadata));

	while(i < config_paginaSize){

		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

		if(auxBloque.size >= size && auxBloque.bitUso == -1){
			printf("Saliendo Pagina Heap Bloque Suficiente\n");
			free(buffer);
			return 1;
		}

		else{
			i = i + sizeof(t_bloqueMetadata) + auxBloque.size;
		}
	}
	printf("Saliendo Pagina Heap Bloque Suficiente\n");
	free(buffer);
	return -1;
}

void liberarBloqueHeap(int pid, int pagina, int offset){
	log_info(loggerConPantalla,"Liberando bloque de memoria dinamica--->PID:%d",pid);
	int i = 0;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));



	t_bloqueMetadata bloque;
	void *buffer=malloc(sizeof(t_bloqueMetadata));
	//buffer = (t_bloqueMetadata*) leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata));

	buffer = leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata));
	memcpy(&bloque,buffer,sizeof(t_bloqueMetadata));

	printf("Leo:\n");
	printf("Pagina:%d\n",pagina);
	printf("Offset:%d\n",i);
	printf("BitUso:%d\n",bloque.bitUso);
	printf("Size:%d\n",bloque.size);
	bloque.bitUso = -1;
	actualizarLiberar(pid,bloque.size);


	memcpy(buffer,&bloque,sizeof(t_bloqueMetadata));
	escribirEnMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata),buffer);


	while(i < list_size(listaAdmHeap))
		{
			aux = list_get(listaAdmHeap,i);
			if(aux->pagina == pagina && aux->pid == pid){
				aux->sizeDisponible = aux->sizeDisponible + bloque .size;
				list_replace(listaAdmHeap,i,aux);
				break;
			}
			i++;
		}
}

#endif /* HEAP_H_ */
