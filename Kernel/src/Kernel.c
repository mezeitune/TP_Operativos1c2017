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

//int recibirConexion(int socket_servidor); // No se usa en este modulo. Va a servir cuando limpiemos codigo del main y lo pasemos a esta funcion. Habria que cambiarla toda basicamente
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socketAceptado, char *orden);
void *get_in_addr(struct sockaddr *sa);
void nuevaOrdenDeAccion(int puertoCliente, char* nuevaOrden);
void selectorConexiones(int socket);


char *ipConsola;
char *ipCPU;
char *ipMemoria;
char *ipFileSys;

char *puertoCPU;
char *puertoMemoria;
char *puertoConsola;
char *puertoFileSys;

char *puertoServidor;
char *ipServidor;

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

//------------Sockets unicos globales--------------------//
int socketMemoria;
int socketFyleSys;
int socketServidor; // Para CPUs y Consolas
//------------------------------------------------------//

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();

	socketServidor = crear_socket_servidor(ipServidor, puertoServidor);
	socketMemoria = crear_socket_cliente(ipMemoria, puertoMemoria);
	socketFyleSys = crear_socket_cliente(ipFileSys, puertoFileSys);

	while(1){
	//Multiplexa las conexiones.
		selectorConexiones(socketServidor);
	}
	return 0;
}


void connectionHandler(int socketAceptado, char *orden) {// Recibe un char* para tener la variable modificada cuando vuelva a selectorConexiones()
	char *buffer;


	if(orden == '\0'){/*Si la orden es '\0' o sea si no fue la PrimerOrden de todas las conexiones, recibe una orden nueva porque la primerorden la seteamos en selectorConexiones(),
	 	 	 	 	 	 Pero despues sin esto no podemos cambiarla, si es que queremos trabajar con las consolas en forma "paralela"*/
		nuevaOrdenDeAccion(socketAceptado, orden);
	}

	printf("El nuevo cliente %d ha enviado la orden: %c\n", socketAceptado, *(char*)orden);
		switch (*(char*)orden) {
			case 'I':

			/* Solo de prueba para ver si el FS y la memoria reciben del Kernel
			//enviar(socketMemoria, (void*)&orden, sizeof(char));
			//enviar(socketFyleSys, (void*)&orden, sizeof(char));
			  */

				printf("Se ha avisado que un archivo esta por enviarse\n");

				if ((buffer = recibir_string(socketAceptado)) == NULL) { // Por si la consola se sale justo aca, asi no rompe todo el Kernel
					printf("ERROR: La consola se ha desconectado\n");
					return;
				}

				printf("\nEl mensaje es: \"  %s \"\n", buffer);

				free(buffer);
				break;

			default:
				if(*orden == '\0') break;/*Esta para que no printee cuando se envia la "orden extra", esto de la orden extra es como un bug que no tengo idea de donde sale,
				 	 	 	 	 	 	 cuando lo prueben comenten esta linea y van a ver lo que digo*/

				printf("ERROR: Orden %c no definida\n", *(char*)orden);
				break;
		}
	*orden = '\0';//Sin esto recive una "orden extra" y rompe, agregandolo, me aseguro que esa "orden extra" vaya al default para que todo siga funcionando como deberia
	return;//Retorna a selectorConexiones() apenas se haya recibido una orden desde la consola para dar lugar a las otras consolas/CPUs

}

void nuevaOrdenDeAccion(int socketCliente, char* nuevaOrden) {
		printf("\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		printf("El cliente %d ha enviado la orden: %c\n", socketCliente, *nuevaOrden);
}

void selectorConexiones(int socket) {

	int fdMax;        // es para un contador de los sockets que hay
	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	int i,j;        // Contador para las iteracion dentro del FDSET
	int nbytes; // El tamanio de los datos que se recibe por recv
	char orden;
	char remoteIP[INET6_ADDRSTRLEN];

	socklen_t addrlen;
	fd_set master;    // master file descriptor list
	fd_set readFds;  // temp file descriptor list for select()

	// listen
	if (listen(socket, 10) == -1) {
		perror("listen");
		exit(1);
	}

	FD_SET(socket, &master); // add the listener to the master set
	fdMax = socket; // keep track of the biggest file descriptor so far, it's this one

	// main loop
	for (;;) {
		readFds = master; // copy it
		if (select(fdMax + 1, &readFds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(2);
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdMax; i++) {
			if (FD_ISSET(i, &readFds)) { // we got one!!
				if (i == socket) {
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(socket, (struct sockaddr *) &remoteaddr,&addrlen);

					if (newfd == -1) {
						perror("accept");
					} else {
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdMax) {    // keep track of the max
							fdMax = newfd;
						}
						printf("selectserver: new connection from %s on " "socket %d\n\n",inet_ntop(remoteaddr.ss_family,	get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), newfd);
					}
				}

				else {
					// handle data from a client

					if ((nbytes = recv(i, &orden, sizeof orden, 0) <= 0)) { // Aca se carga el buffer con el mensaje. Actualmente no lo uso

						// got error or connection closed by client
						if (nbytes == 0) {
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else {
						// we got some data from a client
						for(j = 0; j <= fdMax; j++) {//Rota entre las conexiones
							if (FD_ISSET(j, &master)) {
								if (j != socket && j != i) {
										connectionHandler(i, &orden);
						        }
						    }
						}
					}
				}
			} // END handle data from client
		} // END got new incoming connection
	} // END looping through file descriptors
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void leerConfiguracion(char* ruta) {

	configuracion_kernel = config_create(ruta);

	ipServidor = config_get_string_value(configuracion_kernel, "IP_SERVIDOR");
	puertoServidor = config_get_string_value(configuracion_kernel,"PUERTO_SERVIDOR");
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
	stackSize = config_get_string_value(configuracion_kernel, "STACK_SIZE");
}

void imprimirConfiguraciones() {
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP CONSOLA:%s\nPUERTO CONSOLA:%s\nIP CPU:%s\nPUERTO CPU:%s\nIP FS:%s\nPUERTO FS:%s\n",ipMemoria,puertoMemoria,ipConsola,puertoConsola,ipCPU,puertoCPU,ipFileSys,puertoFileSys);
	printf("---------------------------------------------------\n");
	printf(	"QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%s\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%s\n",	quantum, quantumSleep, algoritmo, gradoMultiProg, semIds, semInit, sharedVars, stackSize);
	printf("---------------------------------------------------\n");

}
