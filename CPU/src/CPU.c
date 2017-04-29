/*
 ============================================================================
 Name        : CPU.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
#include <parser/metadata_program.h>
#include <pthread.h>
#include "conexiones.h"
#include <arpa/inet.h>

void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socketKernel);
void* conexionMemoria (int socketMemoria);
t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;

//------------------Sockets Globales-------//
int socketMemoria;
int socketKernel;
//-----------------------------------------//

pthread_t HiloConexionMemoria;

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);

	int err = pthread_create(&HiloConexionMemoria, NULL, &conexionMemoria,(void*) socketMemoria);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		else
			printf("\n Thread created successfully\n");

	pthread_join(HiloConexionMemoria, NULL);


	connectionHandler(socketKernel);

	return 0;
}

void* conexionMemoria (int socketMemoria){

	//analizadorLinea("a = b + 3", AnSISOP_funciones *AnSISOP_funciones, AnSISOP_kernel *AnSISOP_funciones_kernel);
	while(1){
		connectionHandler(socketMemoria);
	}
	return 0;

}


void connectionHandler(int socket){

	char orden;


	char* mensajeRecibido;

	int paginaAPedir=0;
	int offset=0;
	int pid=1;
	int size=46;

	char comandoSolicitar = 'S';

	while(1){
		while(orden != 'Q'){

			printf("Ingresar orden:\n");
			scanf(" %c", &orden);

			switch(orden){
				case 'S':
					send(socketMemoria,&comandoSolicitar,sizeof(char),0);
					send(socketMemoria,&pid,sizeof(int),0);
					send(socketMemoria,&paginaAPedir,sizeof(int),0);
					send(socketMemoria,&offset,sizeof(int),0);
					send(socketMemoria,&size,sizeof(int),0);

					mensajeRecibido = recibir_string(socketMemoria);
					printf("El mensaje recibido de la Memoria es : %s\n" , mensajeRecibido);

					break;
				case 'Q':
					printf("Se ha desconectado el cliente\n");
					exit(1);
					break;
				default:
					printf("ERROR, Orden %c no definida\n", orden);
					break;
			}
		}
			orden = '\0';
	}
}

void leerConfiguracion(char* ruta) {
	configuracion_memoria = config_create(ruta);
	ipMemoria = config_get_string_value(configuracion_memoria, "IP_MEMORIA");
	puertoKernel= config_get_string_value(configuracion_memoria, "PUERTO_KERNEL");
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO_MEMORIA");
	ipKernel = config_get_string_value(configuracion_memoria,"IP_KERNEL");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nPUERTO KERNEL:%s\nIP KERNEL:%s\nPUERTO MEMORIA:%s\nIP MEMORIA:%s\n",puertoKernel,ipKernel,puertoMemoria,ipMemoria);
	printf("---------------------------------------------------\n");
}

