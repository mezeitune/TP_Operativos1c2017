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
#include "sincronizacion.h"
#include "excepeciones.h"

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

typedef struct{
	int pid;
	int size;
	int socket;
}t_alocar;

t_list* listaAdmHeap;

void handlerExpropiado(int signal);


void reservarEspacioHeap(t_alocar* data);
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

void reservarEspacioHeap(t_alocar* data){
	log_info(loggerConPantalla,"Reservando espacio de memoria dinamica--->PID:%d",data->pid);
	int resultadoEjecucion;

	signal(SIGUSR1,handlerExpropiado);

	t_punteroCpu* puntero = malloc(sizeof(t_punteroCpu));

	pthread_mutex_lock(&mutexMemoria);
	puntero->pagina = verificarEspacioLibreHeap(data->size, data->pid);
	pthread_mutex_unlock(&mutexMemoria);

	if(puntero->pagina  == -1){

		puntero->pagina = obtenerPaginaSiguiente(data->pid);

		pthread_mutex_lock(&mutexMemoria);
		resultadoEjecucion = reservarPaginaHeap(data->pid,puntero->pagina);
		pthread_mutex_unlock(&mutexMemoria);

					if(resultadoEjecucion < 0){
							excepcionCantidadDePaginas(data->socket,data->pid);
							return;
					}

		aumentarPaginasHeap(data->pid);
		}

	pthread_mutex_lock(&mutexMemoria);
	puntero->offset = reservarBloqueHeap(data->pid, data->size, puntero->pagina);
	pthread_mutex_unlock(&mutexMemoria);

	//printf("\nPagina que se le da para ese espacio de memoria:%d\n",puntero->pagina);
	send(data->socket,&resultadoEjecucion,sizeof(int),0);
	send(data->socket,&puntero->pagina,sizeof(int),0);
	send(data->socket,&puntero->offset,sizeof(int),0);
	free(data);
}

void handlerExpropiado(int signal){

	if(signal==SIGUSR1){
	log_error(loggerConPantalla,"Un servicio de Alocar se ha abortado porque el proceso debio ser expropiado");
	int valor;
	pthread_exit(&valor);
	}

}


int verificarEspacioLibreHeap(int size, int pid){
	log_info(loggerConPantalla,"Verificando espacio libre en Heap--->PID:%d",pid);
	int i = 0;
	t_adminBloqueHeap* aux;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = (t_adminBloqueHeap*) list_get(listaAdmHeap,i);
		pthread_mutex_unlock(&mutexListaAdminHeap);

		//printf("i=%d\n",i);
		//printf("sizeDisponible=%d\n",aux->sizeDisponible);
		//printf("pid=%d\n",aux->pid);
		if(aux->sizeDisponible >= size && aux->pid == pid)
		{
			/**TODO: Mutex para compactar?*/
			compactarPaginaHeap(aux->pagina,aux->pid);

			if(paginaHeapBloqueSuficiente(i,aux->pagina,aux->pid,size) > 0){
				return aux->pagina;
			}
		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);
	return -1;
}


int reservarPaginaHeap(int pid,int pagina){ //Reservo una página de heap nueva para el proceso
	log_info(loggerConPantalla,"Reservando pagina de heap--->PID:%d",pid);
	int resultadoEjecucion;
	t_bloqueMetadata aux ;

	void* buffer=malloc(sizeof(t_bloqueMetadata));
	aux.bitUso = -1;
	aux.size = config_paginaSize - sizeof(t_bloqueMetadata);
	memcpy(buffer,&aux,sizeof(t_bloqueMetadata));

	resultadoEjecucion=reservarPaginaEnMemoria(pid);
	if(resultadoEjecucion < 0) return -1;

	resultadoEjecucion=escribirEnMemoria(pid,pagina,0,sizeof(t_bloqueMetadata),buffer);  //Para indicar que está sin usar y que tiene tantos bits libres para utilizarse


	t_adminBloqueHeap* bloqueAdmin=malloc(sizeof(t_adminBloqueHeap));
	bloqueAdmin->pagina = pagina;
	bloqueAdmin->pid = pid;
	bloqueAdmin->sizeDisponible = aux.size;
	/*printf("Pagina Reservada:%d\n",bloqueAdmin->pagina);
	printf("PID Proceso Reservado:%d\n",bloqueAdmin->pid);
	printf("Size Disponible Pagina Reservada:%d\n",bloqueAdmin->sizeDisponible);
	*/
	list_add(listaAdmHeap, bloqueAdmin);
	free(buffer);
	log_info(loggerConPantalla,"Pagina de heap %d reservada--->PID:%d",pagina,pid);
	return resultadoEjecucion;
}


void compactarPaginaHeap(int pagina, int pid){
	log_info(loggerConPantalla,"Compactando pagina de heap %d--->PID :%d",pagina,pid);
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

		/*printf("Actual bitUso=%d\n",actual.bitUso);
		printf("Actual size=%d\n",actual.size);
		printf("Siguiente bitUso=%d\n",siguiente.bitUso);
		printf("Siguiente size=%d\n",siguiente.size);
*/
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
	log_info(loggerConPantalla,"Pagina de heap %d compactada--->PID :%d",pagina,pid);
}

void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido){
	escribirEnMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size,contenido);
}

void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido){
	*contenido = leerDeMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size);
}

int reservarBloqueHeap(int pid,int size,int pagina){
	log_info(loggerConPantalla,"Reservando bloque en pagina heap:%d --->PID:%d",pagina,pid);
	t_bloqueMetadata auxBloque;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));
	int offset=0;
	int i = 0;
	int sizeLibreViejo;
	void *buffer=malloc(sizeof(t_bloqueMetadata));

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pid){
			if(size + sizeof(t_bloqueMetadata) >= aux->sizeDisponible){
				pthread_mutex_unlock(&mutexListaAdminHeap);
				printf("\nEntre a un error\n");
				return -16; //CODIGO DE ERROR - NO SE PUEDE HACER ESTA RESERVA
			}
			else{
				aux->sizeDisponible = aux->sizeDisponible - size - sizeof(t_bloqueMetadata);
				list_replace(listaAdmHeap,i,aux);
				break;
			}
		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);
	i = 0;

	while(i < config_paginaSize){


		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

		/*printf("Leo:\n");
		printf("Pagina:%d\n",pagina);
		printf("Offset:%d\n",i);
		printf("BitUso:%d\n",auxBloque.bitUso);
		printf("Size:%d\n",auxBloque.size);
*/
		if(auxBloque.size >= size + sizeof(t_bloqueMetadata) && auxBloque.bitUso == -1){
			sizeLibreViejo = auxBloque.size;
			auxBloque.bitUso = 1;
			auxBloque.size = size;
			memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));
			/*printf("Escribo:\n");
			printf("BitUso:%d\n",auxBloque.bitUso);
			printf("Size:%d\n",auxBloque.size);
			printf("Pagina:%d\n",pagina);
			printf("Offset:%d\n",i);
*/
			escribirEnMemoria(pid,pagina,i,sizeof(t_bloqueMetadata),buffer); //Escribo y reservo el metadata que se quiere reservar

			offset = i;

			auxBloque.bitUso = -1;
			auxBloque.size = sizeLibreViejo - size - sizeof(t_bloqueMetadata);

			/*printf("Escribo:\n");
			printf("Bit uso: %d\n",auxBloque.bitUso);
			printf("Size disponible: %d\n",auxBloque.size);
			printf("Pagina:%d\n",pagina);
			printf("Offset:%d",i);
			*/
			memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));

			escribirEnMemoria(pid,pagina,i+sizeof(t_bloqueMetadata)+size,sizeof(t_bloqueMetadata),buffer); //Anuncio cuanto espacio libre queda en el heap en el siguiente metadata


			break;
		}
		else{
			i = i + sizeof(t_bloqueMetadata) + auxBloque.size;
		}

	}
	free(buffer);
	//free(auxBloque);
	//free(aux);
	log_info(loggerConPantalla,"Bloque de pagina heap %d reservado --->PID:%d",pagina,pid);
	return offset;
}

void destruirPaginaHeap(int pidProc, int pagina){ //Si quiero destruir una página específica de la lista
	t_adminBloqueHeap* aux;
	int i = 0;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
			break;
		}
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);

}

void destruirTodasLasPaginasHeapDeProceso(int pidProc){ //Elimino todas las estructuras administrativas de heap asociadas a un PID
	t_adminBloqueHeap* aux;
	int i = 0;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);

}

int paginaHeapBloqueSuficiente(int posicionPaginaHeap,int pagina,int pid ,int size){
	printf("Pagina Heap Bloque Suficiente\n");
	int i = 0;


	t_bloqueMetadata auxBloque;
	void *buffer= malloc(sizeof(t_bloqueMetadata));

	while(i < config_paginaSize){

		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

		if(auxBloque.size >= size + sizeof(t_bloqueMetadata) && auxBloque.bitUso == -1){
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

	pthread_mutex_lock(&mutexMemoria);
	buffer = leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata));
	pthread_mutex_unlock(&mutexMemoria);

	memcpy(&bloque,buffer,sizeof(t_bloqueMetadata));

	/*printf("Leo:\n");
	printf("Pagina:%d\n",pagina);
	printf("Offset:%d\n",i);
	printf("BitUso:%d\n",bloque.bitUso);
	printf("Size:%d\n",bloque.size);
	*/
	bloque.bitUso = -1;
	/*TODO: Poder saber bien cuanto estoy liberando*/
	printf("\n\nEstoy liberando:%d\n\n",bloque.size);
	actualizarLiberar(pid,bloque.size);
	memcpy(buffer,&bloque,sizeof(t_bloqueMetadata));

	pthread_mutex_lock(&mutexMemoria);
	escribirEnMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata),buffer);
	pthread_mutex_unlock(&mutexMemoria);

	pthread_mutex_lock(&mutexListaAdminHeap);
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
	pthread_mutex_unlock(&mutexListaAdminHeap);
}

#endif /* HEAP_H_ */
