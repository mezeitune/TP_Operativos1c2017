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

int recibirConexion(int socket_servidor); // No se usa en este modulo. Va a servir cuando limpiemos codigo del main y lo pasemos a esta funcion. Habria que cambiarla toda basicamente
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void *connectionHandler(int socketAceptado);
void *get_in_addr(struct sockaddr *sa);
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


/*
//the thread function
void *sock_Consola();
//the thread function
void *sock_CPU();
//the thread function
void *sock_FS();
//the thread function
void *sock_Memoria();

int socket_FS;
int socket_Mem;
int socket_CPU;
int socket_Consola;
*/

int main(void)
{
     int fdMax;        // Creo que es para un contador de los sockets que hay
     int newfd;        // newly accept()ed socket descriptor
     struct sockaddr_storage remoteaddr; // client address
     socklen_t addrlen;
     fd_set master;    // master file descriptor list
	 fd_set readFds;  // temp file descriptor list for select()
	 int i, j;        // Contadores para las iteraciones dentro del FDSET
	 int nbytes; // El tamanio de los datos que se recibe por recv
	 char buffer[256];    // buffer for client data

	 char remoteIP[INET6_ADDRSTRLEN];


	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	int socketServidor = crear_socket_servidor(ipServidor ,puertoServidor);

	 // listen
	 if (listen(socketServidor, 10) == -1) {
	        perror("listen");
	        exit(1);
	    }

	FD_SET(socketServidor, &master); // add the listener to the master set

	fdMax = socketServidor; // keep track of the biggest file descriptor so far, it's this one


    // main loop
    for(;;) {
        readFds = master; // copy it
        if (select(fdMax+1, &readFds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(2);
        }

        // run through the existing connections looking for data to read
		for(i = 0; i <= fdMax; i++) {
			if (FD_ISSET(i, &readFds)) { // we got one!!
				if (i == socketServidor) {
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(socketServidor,(struct sockaddr *)&remoteaddr,&addrlen);

					 if (newfd == -1) {
					                   perror("accept");
					                    } else {
					                        FD_SET(newfd, &master); // add to master set
					                        if (newfd > fdMax) {    // keep track of the max
					                            fdMax = newfd;
					                        }
					                        printf("selectserver: new connection from %s on ""socket %d\n",inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, INET6_ADDRSTRLEN),newfd);
					                    	}
				}

				else {
					  // handle data from a client

						if ((nbytes = recv(i,buffer,sizeof buffer, 0) <= 0)) { // Aca se carga el buffer con el mensaje. Actualmente no lo uso

							// got error or connection closed by client
							if (nbytes == 0) {
								// connection closed
								printf("selectserver: socket %d hung up\n", i);
							} else {
								perror("recv");
							}
							close(i); // bye!
							FD_CLR(i, &master); // remove from master set
						} else {
							// we got some data from a client
							for(j = 0; j <= fdMax; j++) { // Con este for se recorre la lista de sockets qe se conectaron al Kernel.
								// send to everyone!
								if (FD_ISSET(j, &master)) {
									// except the listener and ourselves
									if (j != socketServidor) {
										printf("\n----connectionHandler assigned to socket %d----\n",j);

										connectionHandler(j);

										/*if (send(j, buffer, nbytes, 0) == -1) { //Esto es para mandarle el msj recibido a todo mis clientes. Era terrible idea para el check 1
											perror("send");
										}
										*/
									}
								}
							}
						}
					} // END handle data from client
				} // END got new incoming connection
			} // END looping through file descriptors
		} // END for(;;)--and you thought it would never end!

			    return 0;
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void *connectionHandler(int socketAceptado) {
	//Get the socket descriptor
	char orden;
	orden=nuevaOrdenDeAccion(socketAceptado);
	while(orden != 'Q')
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
			/*case 'C':
					printf("Esperando mensaje\n");

					enviar(socket_CPU, (void*) &orden, sizeof(char));//Le avisa a la CPU que le va a mandar un string
					enviar(socket_FS, (void*) &orden, sizeof(char));//Le avisa al FS que le va a mandar un string
					enviar(socket_Mem, (void*) &orden, sizeof(char));//Le avisa a la memoria que le va a mandar un string

					buffer = recibir_string(sock);//Espera y recibe string desde la consola

					enviar_string(socket_FS, buffer);//envia mensaje al FS
					enviar_string(socket_Mem, buffer);//envia mensaje a la Memoria
					enviar_string(socket_CPU, buffer);//envia mensaje a la CPU

					printf("\nEl mensaje es: \"  %s \"\n", buffer);
					free(buffer);
					break;
					*/
			default:
				printf("ERROR: Orden %c no definida\n",orden);
				break;
			}
			orden=nuevaOrdenDeAccion(socketAceptado);
		}

	return 0;

}

char nuevaOrdenDeAccion(int socketCliente) {
	char* orden;
	printf("\n--Esperando una orden del cliente-- \n");
	orden = recibir(socketCliente);
	printf("El cliente ha enviado la orden: %c\n", *orden);
	return *orden;
}

void leerConfiguracion(char* ruta) {

	configuracion_kernel = config_create(ruta);

	ipServidor=config_get_string_value(configuracion_kernel,"IP_SERVIDOR");
	puertoServidor=config_get_string_value(configuracion_kernel, "PUERTO_SERVIDOR");
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



// MAIN CON HILOS DE EJECUCION
/*
socket_FS = crear_socket_cliente(ipFileSys, puertoFileSys);
socket_Mem = crear_socket_cliente(ipMemoria, puertoMemoria);

if (pthread_create(&threadCPU, NULL, sock_CPU, (void*) NULL) < 0) {
		perror("could not create thread");
		return 1;
}

if (pthread_create(&threadConsola, NULL, sock_Consola, (void*) NULL) < 0) {
	perror("could not create thread");
	return 1;
}

pthread_join( threadCPU, NULL);
pthread_join( threadConsola, NULL);

*/

/*
void* sock_Consola() {
int socket_servidorConsola = crear_socket_servidor(ipConsola, puertoConsola);
while(1)
{
	socket_Consola = recibirConexion(socket_servidorConsola);

	if (pthread_create(&thread_id, NULL, connection_handler,(void*) &socket_Consola) < 0) {
		perror("could not create thread");
	}
}
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

int socket_servidorCPU = crear_socket_servidor(ipCPU, puertoCPU);

while(1)
{
	socket_CPU = recibirConexion(socket_servidorCPU);
	if (pthread_create(&thread_id, NULL, connection_handler,(void*) &socket_CPU) < 0) {
		perror("could not create thread");
	}
}
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

socket_aceptado = accept(socket_servidor,(struct sockaddr *) &their_addr, &addr_size);
contadorConexiones ++;
printf("\n----------Nueva Conexion aceptada numero: %d ---------\n",contadorConexiones);


if (socket_aceptado == -1) {
		close(socket_servidor);
		printf("Error al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}
*/
