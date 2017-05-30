/*
 * hiloPrograma.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "conexiones.h"
#include <time.h>


typedef struct {
	int pid;
	struct tm* fechaInicio;
	int cantImpresiones;
	int socketHiloKernel;
	pthread_t idHilo;
}t_hiloPrograma;


pthread_mutex_t mutex_crearHilo;
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
	list_add(listaHilosProgramas,nuevoPrograma);
}

void* iniciarPrograma(int* socketHiloKernel){
	char *ruta = (char*) malloc(200 * sizeof(char));

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
	int pid=0;
	int size;
	char* mensaje;

	recv(socketHiloKernel, &pid, sizeof(int), 0);
	log_info(loggerConPantalla,"\nEl PID asignado es: %d \n", pid);

	cargarHiloPrograma(pid,socketHiloKernel);
	/* Se queda escuchando los mensaje para imprimir */
	while(1){
		printf("Hilo programa--- PID: %d ----SOCKET: %d ----- escuchando Kernel\n", pid, socketHiloKernel);
		pthread_mutex_unlock(&mutex_crearHilo);
		recv(socketHiloKernel,&size,sizeof(int),0);
		mensaje = malloc(size);
		printf("El tamano del mensaje a recibir es: %d\n", size);
		recv(socketHiloKernel,(void*)mensaje,size,0);
		printf("%s\n",mensaje);
	}

	printf("Termine de recibir los datos del kernel\n");
}


void cargarHiloPrograma(int pid, int socket){

	_Bool verificarSocket(t_hiloPrograma* hiloPrograma){
					return (hiloPrograma->socketHiloKernel ==socket);
					}
	t_hiloPrograma* hiloPrograma= list_remove_by_condition(listaHilosProgramas,(void*)verificarSocket);
	hiloPrograma->pid = pid;
	list_add(listaHilosProgramas,hiloPrograma);
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

	mensaje = malloc(sizeof(int) + sizeof(char) + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f);
	strcat(bufferArchivo,"\0");
	tamanioArchivo += sizeof(char);

	printf("El tamano del archivo a enviar es: %d\n", tamanioArchivo);
	printf("El archivo a enviar es:\n %s\n", bufferArchivo);

	memcpy(mensaje, &comandoIniciarPrograma,sizeof(char));
	memcpy(mensaje + sizeof(char), &tamanioArchivo, sizeof(int));
	memcpy(mensaje + sizeof(char) + sizeof(int), bufferArchivo, tamanioArchivo);
	printf("Enviando codigo ANSISOP por socket: %d\n", socketHiloKernel);
	send(socketHiloKernel, mensaje, tamanioArchivo + sizeof(int) + sizeof(char)  , 0);
	log_info(loggerConPantalla,"\nEl programa ANSISOP ha sido enviado al Kernel\n");

	free(bufferArchivo);
	free(mensaje);

	return 0;
}
