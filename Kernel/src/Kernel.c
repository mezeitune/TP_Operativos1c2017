/*
 ============================================================================
 Name        : Kernel.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Kernel Module
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
#include <pthread.h>
#include <commons/conexiones.h>

int recibirConexion(int socket_servidor);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void *connection_handler(void *socket_desc);
char nuevaOrdenDeAccion(int puertoCliente);
int contadorConexiones=0;

char *ipCPU;
char *ipMemoria;
char *ipConsola;
char *ipFileSys;

char *puertoCPU;
char *puertoMemoria;
char *puertoConsola;
char *puertoFileSys;

char *quantum;
char *quantumSleep;
char *algoritmo;
char *gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;
char *stackSize;
pthread_t thread_id, threadCPU, threadConsola, threadMemoria, threadFS;
t_config* configuracion_kernel;

//the thread function
void *sock_Consola();
//the thread function
void *sock_CPU();
//the thread function
void *sock_FS();
//the thread function
void *sock_Memoria();

int main(void)
{
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();

	if (pthread_create(&threadCPU, NULL, sock_CPU, (void*) NULL) < 0) {
			perror("could not create thread");
			return 1;
		}

	if (pthread_create(&threadConsola, NULL, sock_Consola, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	if (pthread_create(&threadFS, NULL, sock_FS, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	if (pthread_create(&threadMemoria, NULL, sock_Memoria, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	pthread_join( threadMemoria, NULL);
	pthread_join( threadFS, NULL);
	pthread_join( threadCPU, NULL);
	pthread_join( threadConsola, NULL);

	 return 0;
}

void* sock_Consola() {
	int socket_servidorProg = crear_socket_servidor(ipConsola, puertoConsola);
	recibirConexion(socket_servidorProg);
}

void* sock_FS() {
	char orden;
	int socket_FS = crear_socket_cliente(ipFileSys, puertoFileSys);
	while (orden != 'Q') {
		scanf(" %c", &orden);
		enviar(socket_FS, (void*) &orden, sizeof(char));
	}
}

void* sock_Memoria() {
	char orden;
	int socket_Mem = crear_socket_cliente(ipMemoria, puertoMemoria);
	while (orden != 'Q') {
		scanf("%c", &orden);
		enviar(socket_Mem, (void*) &orden, sizeof(char));
	}
}

void* sock_CPU() {
	int socket_servidor = crear_socket_servidor(ipCPU, puertoCPU);
	recibirConexion(socket_servidor);

}

int recibirConexion(int socket_servidor) {
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	addr_size = sizeof(their_addr);
	int socket_aceptado;

	int estado = listen(socket_servidor, 5);

	if (estado == -1) {
		printf("Error al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}

	if (estado == 0) {
		printf("Se puso el socket en listen\n");
	}

	addr_size = sizeof(their_addr);

	while ((socket_aceptado = accept(socket_servidor,(struct sockaddr *) &their_addr, &addr_size))) {
		contadorConexiones ++;
		printf("\n----------Nueva Conexion aceptada numero: %d ---------\n",contadorConexiones);

		if (pthread_create(&thread_id, NULL, connection_handler,(void*) &socket_aceptado) < 0) {
			perror("could not create thread");
			return 1;
		}
		printf("----------Handler asignado a (%d) ---------\n",contadorConexiones);
	}

		if (socket_aceptado == -1) {
				close(socket_servidor);
				printf("Error al aceptar conexion\n");
				return 1;
			}

		return socket_aceptado;

		//Now join the thread , so that we dont terminate before the thread
	    //pthread_join( thread_id , NULL);
}

void *connection_handler(void *socket_desc) {
	//Get the socket descriptor
	int sock = *(int*) socket_desc;
	char *buffer;
	char orden;
	int socket_FS = crear_socket_cliente(ipFileSys, puertoFileSys);//Crea socket para FS
	int socket_Mem = crear_socket_cliente(ipMemoria, puertoMemoria);//Crea socket para Memoria
	//int socket_CPU = crear_socket_servidor(ipCPU, puertoCPU);//Crea socket para CPU No funca


	while((orden=nuevaOrdenDeAccion(sock)) != 'Q')
		{
			switch(orden)
			{
			case 'I': printf("Usted marco la I\n");
				break;
			case 'S': printf("Usted marco al S\n");
				break;
			case 'A': printf("Usted marco la A\n");
				break;
			case 'G': printf("Usted marco la G\n");
				break;
			case 'C':
					printf("Esperando mensaje\n");

				//	enviar(socket_CPU, (void*) &orden, sizeof(char));//Le avisa a la CPU que le va a mandar un string No funca
					enviar(socket_FS, (void*) &orden, sizeof(char));//Le avisa al FS que le va a mandar un string
					enviar(socket_Mem, (void*) &orden, sizeof(char));//Le avisa a la memoria que le va a mandar un string

					buffer = recibir_string(sock);//Espera y recibe string desde la consola

					enviar_string(socket_FS, buffer);//envia mensaje al FS
					enviar_string(socket_Mem, buffer);//envia mensaje a la Memoria
					//enviar_string(socket_CPU, buffer);//envia mensaje a la CPU No funca

					printf("\nEl mensaje es: \"  %s \"\n", buffer);
					free(buffer);
					break;
			default:
				printf("ERROR: Orden %c no definida\n",orden);
				break;
			}

		}
	printf("\nUn Cliente se ha desconectado\n");
	fflush(stdout);
	return 0;

}

char nuevaOrdenDeAccion(int socketCliente) {
	char *buffer;
	printf("\n--Esperando una orden del cliente-- \n");
	buffer = recibir(socketCliente);
	//int size_mensaje = sizeof(buffer);
	if (buffer == NULL) {
		return 'Q';
		//puts("Client disconnected");
		//fflush(stdout);
	} else if (buffer == -1) {
		return 'X';
		perror("recv failed");
	}

	printf("El cliente ha enviado la orden: %c\n", *buffer);
	return *buffer;
}

void leerConfiguracion(char* ruta) {

	configuracion_kernel = config_create(ruta);

	puertoCPU = config_get_string_value(configuracion_kernel, "PUERTO_CPU");
	ipMemoria = config_get_string_value(configuracion_kernel, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel, "IP_FS");
	puertoFileSys = config_get_string_value(configuracion_kernel, "PUERTO_FS");
	quantum = config_get_string_value(configuracion_kernel, "QUANTUM");
	quantumSleep = config_get_string_value(configuracion_kernel,"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion_kernel, "ALGORTIMO");
	gradoMultiProg = config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION");
	semIds = config_get_string_value(configuracion_kernel, "SEM_IDS");
	semInit = config_get_string_value(configuracion_kernel, "SEM_INIT");
	sharedVars = config_get_string_value(configuracion_kernel, "SHARED_VARS");
	puertoConsola = config_get_string_value(configuracion_kernel,"PUERTO_CONSOLA");
	ipConsola = config_get_string_value(configuracion_kernel,"IP_CONSOLA");
	ipCPU = config_get_string_value(configuracion_kernel,"IP_CPU");
	stackSize = config_get_string_value(configuracion_kernel,"STACK_SIZE");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP CONSOLA:%s\nPUERTO CONSOLA:%s\nIP CPU:%s\nPUERTO CPU:%s\nIP FS:%s\nPUERTO FS:%s\n",ipMemoria,puertoMemoria,ipConsola,puertoConsola,ipCPU,puertoCPU,ipFileSys,puertoFileSys);
		printf("---------------------------------------------------\n");
		printf("QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%s\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%s\n",quantum,quantumSleep,algoritmo,gradoMultiProg,semIds,semInit,sharedVars,stackSize);
		printf("---------------------------------------------------\n");

}
