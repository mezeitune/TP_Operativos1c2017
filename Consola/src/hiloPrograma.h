/*
 * hiloPrograma.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "conexiones.h"
#include <time.h>
#include <semaphore.h>


typedef struct {
	int pid;
	struct tm* fechaInicio;
	int cantImpresiones;
	int socketHiloKernel;
	pthread_t idHilo;
}t_hiloPrograma;

pthread_mutex_t mutexListaHilos;
pthread_mutex_t mutex_crearHilo;
sem_t sem_crearHilo;
t_list* listaHilosProgramas;

void crearHiloPrograma();
void* iniciarPrograma(int* socketHiloKernel);
void recibirDatosDelKernel(int socketHiloKernel);
int enviarLecturaArchivo(char *ruta,int socketHiloKernel);
void cargarHiloPrograma(int pid, int socket);

void crearHiloPrograma(){
	t_hiloPrograma* nuevoPrograma = malloc(sizeof(t_hiloPrograma));
	nuevoPrograma->socketHiloKernel=  crear_socket_cliente(ipKernel,puertoKernel);
	int err = pthread_create(&nuevoPrograma->idHilo , NULL ,(void*)iniciarPrograma ,&nuevoPrograma->socketHiloKernel);
	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));
    time_t tiempoInicio = time(0);
	nuevoPrograma->fechaInicio=localtime(&tiempoInicio);

	pthread_mutex_lock(&mutexListaHilos);
	list_add(listaHilosProgramas,nuevoPrograma);
	pthread_mutex_unlock(&mutexListaHilos);
}

void* iniciarPrograma(int* socketHiloKernel){
	char *ruta = malloc(200 * sizeof(char));

	printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
	scanf("%s", ruta);

	if ((enviarLecturaArchivo(ruta,*socketHiloKernel)) < 0) {
		log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente\n");
	}
	free(ruta);

	recibirDatosDelKernel(*socketHiloKernel);
	return 0;
}

void recibirDatosDelKernel(int socketHiloKernel){
	int pid;
	int size;

	recv(socketHiloKernel, &pid, sizeof(int), 0);
	log_info(loggerConPantalla,"Al programa ANSISOP en socket: %d se le ha asignado el PID: %d", socketHiloKernel,pid);

	cargarHiloPrograma(pid,socketHiloKernel);
	sem_post(&sem_crearHilo);
	while(1){
		//printf("Hilo programa--- PID: %d ----SOCKET: %d ----- escuchando Kernel\n", pid, socketHiloKernel);
		recv(socketHiloKernel,&size,sizeof(int),0);
		log_info(loggerConPantalla,"Informacion para PID: %d por SOCKET: %d",pid,socketHiloKernel);
		char* mensaje = malloc(size);
		recv(socketHiloKernel,(void*)mensaje,size,0);
		strcpy(mensaje+size,"\0");
		printf("%s\n",mensaje);
		free(mensaje);
	}
}


void cargarHiloPrograma(int pid, int socket){

	_Bool verificarSocket(t_hiloPrograma* hiloPrograma){
					return (hiloPrograma->socketHiloKernel ==socket);
					}
	t_hiloPrograma* hiloPrograma= list_remove_by_condition(listaHilosProgramas,(void*)verificarSocket);
	hiloPrograma->pid = pid;
	pthread_mutex_lock(&mutexListaHilos);
	list_add(listaHilosProgramas,hiloPrograma);
	pthread_mutex_unlock(&mutexListaHilos);
}

int enviarLecturaArchivo(char *ruta,int socketHiloKernel) {
	FILE *f;
	void *mensaje;
	char *bufferArchivo;
	int tamanioArchivo;
	char comandoIniciarPrograma='I';

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo + sizeof(char)); // Pido memoria para leer el contenido del archivo

	if (bufferArchivo == NULL) {
		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria dinamica\n");
		free(bufferArchivo);
		exit(2);
	}
	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f);
	strcat(bufferArchivo,"\0");
	tamanioArchivo += sizeof(char);
	mensaje = malloc(sizeof(int) + sizeof(char) + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	memcpy(mensaje, &comandoIniciarPrograma,sizeof(char));
	memcpy(mensaje + sizeof(char), &tamanioArchivo, sizeof(int));
	memcpy(mensaje + sizeof(char) + sizeof(int), bufferArchivo, tamanioArchivo);
	send(socketHiloKernel, mensaje, tamanioArchivo + sizeof(int) + sizeof(char)  , 0);

	free(bufferArchivo);
	free(mensaje);

	return 0;
}
