/*
 ============================================================================
 Name        : Kernel.c
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
#include <pthread.h>
#include <commons/conexiones.h>

int recibirConexion(int socket_servidor);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void *connection_handler(void *socket_desc);
char nuevaOrdenDeAccion(int puertoCliente);

char *ipCPU;
char *ipMemoria;
char *ipConsola;
char *ipFileSys;

char *puertoCPU; //3001
char *puertoMemoria; //4040
char *puertoConsola; //2001
char *puertoFileSys;

char *quantum;
char *quantumSleep;
char *algoritmo;
char *gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;
char *stackSize;
pthread_t thread_id;
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
	char orden;

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();

	//int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria); //Variable definidas
	//int socket_servidor = crear_socket_servidor(ipMemoria,puertoFileSys);
	//int socket_servidor1 = crear_socket_servidor(ipMemoria,puertoCPU);
	//recibirConexion(socket_servidor1);

	if (pthread_create(&thread_id, NULL, sock_CPU, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}
	if (pthread_create(&thread_id, NULL, sock_Consola, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	if (pthread_create(&thread_id, NULL, sock_FS, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	if (pthread_create(&thread_id, NULL, sock_Memoria, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
	}
	int socket_servidorP = crear_socket_servidor(ipConsola, puertoConsola);
	/*recibirConexion(socket_servidorP);
	char *buffer = recibir_string(socket_servidorP);*/
	//printf("\n%s\n", buffer);


	 int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria); //Variable definidas
	 while(1) {
	 scanf(" %c", &orden);
	 enviar(socket_Memoria,(void*) &orden,sizeof(char));
	 }
	 return 0;
}

void* sock_Consola() {
	int socket_servidorProg = crear_socket_servidor(ipConsola, puertoConsola);
	recibirConexion(socket_servidorProg);
}

void* sock_FS() {
	char orden;
	//int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria);
	int socket_FS = crear_socket_cliente("127.0.0.1", "5001");
	while (orden != 'Q') {
		scanf(" %c", &orden);
		enviar(socket_FS, (void*) &orden, sizeof(char));
	}
	//int socket_servidorFS = crear_socket_servidor(ipFileSys,puertoFileSys);
	//recibirConexion(socket_servidorFS);
}

void* sock_Memoria() {
	char orden;
	//	int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria);
	int socket_Mem = crear_socket_cliente(ipMemoria, puertoMemoria);
	while (orden != 'Q') {
		scanf("%c", &orden);
		enviar(socket_Mem, (void*) &orden, sizeof(char));
	}
}

void* sock_CPU() {
	int socket_servidor = crear_socket_servidor("127.0.0.1", puertoCPU);
	recibirConexion(socket_servidor);
}

int recibirConexion(int socket_servidor) {
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

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

	int socket_aceptado;
	while ((socket_aceptado = accept(socket_servidor,(struct sockaddr *) &their_addr, &addr_size))) {
		puts("Connection accepted");

		if (pthread_create(&thread_id, NULL, connection_handler,(void*) &socket_aceptado) < 0) {
			perror("could not create thread");
			return 1;
		}

		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( thread_id , NULL);
		puts("Handler assigned");
	}

	if (socket_aceptado == -1) {
		close(socket_servidor);
		printf("Error al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}

// Misma funcion que en Memoria. Solo para testear
void *connection_handler(void *socket_desc) {
	//Get the socket descriptor
	int sock = *(int*) socket_desc;
	char * buffer = recibir_string(sock);
	printf("\n%s\n", buffer);

	/*char orden = 'F';
	char *buffer = malloc(sizeof(char) * 200);
	while (orden != 'Q') {
		orden = nuevaOrdenDeAccion(sock);
		switch (orden) {
		case 'I':
			printf("/nI");
			//main_inicializarPrograma();
			break;
		case 'S':
			//main_solicitarBytesPagina();
			break;
		case 'A':
			//main_almacenarBytesPagina();
			break;
		case 'G':
			//main_asignarPaginaAProceso();
			break;
		case 'F':
			//main_finalizarPrograma();
			break;
		case 'Q':
			puts("Cliente desconectado");
			fflush(stdout);
			break;
		case 'X':
			perror("recv failed");
			break;
		case 'J':
			printf("Esperando String\n");
			buffer = recibir_string(sock);
			puts(buffer);

			free(buffer);
			break;
		default:
			printf("Error\n");
			break;
		}
	}*/
	return 0;
}

char nuevaOrdenDeAccion(int puertoCliente) {
	char *buffer;
	printf("Esperando Orden del Cliente\n");
	buffer = recibir(puertoCliente);
	//int size_mensaje = sizeof(buffer);
	if (buffer == NULL) {
		return 'Q';
		//puts("Client disconnected");
		//fflush(stdout);
	} else if (buffer == -1) {
		return 'X';
		//perror("recv failed");
	}
	printf("Orden %c\n", *buffer);
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
