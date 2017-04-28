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
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/conexiones.h>

typedef struct PCB {
	int pid;
	int cantidadPaginas;
	//int* programCounter;
	//int indiceCodigo[2];
	//Indice de etiquetas
	//Indice del Stack
	//int exitCode;
}t_pcb;

typedef struct CONSOLA{
	int pid;
	int consolaId;
}t_consola;


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
int gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;
char *stackSize;
pthread_t thread_id, threadCPU, threadConsola, threadMemoria, threadFS;
t_config* configuracion_kernel;

void inicializarLog();
void escribirLog(char* mensaje);
FILE * logFile;



//----------Planificacion----------//
int crearNuevoProceso(char* buffer,int size);
void encolarProcesoListo(t_pcb *procesoListo);
void inicializarColaListos();
t_list* colaListos;
t_list* colaTerminados;
t_list* listaConsolas;
int contadorPid=0;
//---------PLanificacion-----------//

//---------Conexiones---------------//

void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socketAceptado, char *orden);
void *get_in_addr(struct sockaddr *sa);
void nuevaOrdenDeAccion(int puertoCliente, char* nuevaOrden);
void selectorConexiones(int socket);
//-----------Conexiones------------------//

//------------Sockets unicos globales--------------------//
int socketMemoria;
int socketFyleSys;
int socketServidor; // Para CPUs y Consolas
//------------------------------------------------------//

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	inicializarLog();
	inicializarColaListos();
	listaConsolas = list_create();

	socketServidor = crear_socket_servidor(ipServidor, puertoServidor);
	socketMemoria = crear_socket_cliente(ipMemoria, puertoMemoria);
	socketFyleSys = crear_socket_cliente(ipFileSys, puertoFileSys);

	while(1){
	/*Multiplexor de conexiones. */
		selectorConexiones(socketServidor);
	}


	//Hay que cerrar el log para que lo escriba.
	return 0;
}

void connectionHandler(int socketAceptado, char *orden) {// Recibe un char* para tener la variable modificada cuando vuelva a selectorConexiones()
	void *buffer;
	int bytesARecibir=0;
	int consolaId=0;

	if(orden == '\0'){/*Si la orden es '\0' o sea si no fue la PrimerOrden de todas las conexiones, recibe una orden nueva porque la primerorden la seteamos en selectorConexiones(),
	 	 	 	 	 	 Pero despues sin esto no podemos cambiarla, si es que queremos trabajar con las consolas en forma "paralela"*/
		nuevaOrdenDeAccion(socketAceptado, orden);
	}

	printf("El nuevo cliente %d ha enviado la orden: %c\n", socketAceptado, *(char*)orden);

	switch (*(char*)orden) {

		/*Caso en el que se quiere recibir un archivo empaquetado */
		case 'I':
						printf("Se ha avisado que un archivo esta por enviarse\n");

						recv(socketAceptado,&bytesARecibir, sizeof(int),0); //
						printf("Los bytes a recibir son: %d \n", bytesARecibir);

						buffer = malloc(bytesARecibir); // Pido memoria para recibir el contenido del archivo
						recv(socketAceptado,buffer,bytesARecibir  ,0);
						printf("\n El mensaje recibido es: \" %s \" \n", buffer);

						contadorPid++; // Valor temporal del pid.
						send(socketAceptado,&contadorPid,sizeof(int),0);

						t_consola *infoConsola = malloc(sizeof(t_consola));
						infoConsola->pid=contadorPid;
						infoConsola->consolaId=consolaId;


						if((crearNuevoProceso(buffer,bytesARecibir))<0){ // Aca naceria el planificador de procesos.
							printf("No se puede crear un nuevo proceso en el sistema");
						}
						else {
							list_add(listaConsolas,infoConsola);
						}
						free(infoConsola);
						free(buffer);

						break;
		case 'Q' :
						printf("Se ha solicitado cerrar la consola\n");
						fclose(logFile);
						exit(1);
						break;
			default:
				if(*orden == '\0') break;/*Esta para que no printee cuando se envia la "orden extra", esto de la orden extra es como un bug que no tengo idea de donde sale,
				 	 	 	 	 	 	 cuando lo prueben comenten esta linea y van a ver lo que digo*/

				printf("ERROR: Orden %c no definida\n", *(char*)orden);
				break;
		} // END switch.

	*orden = '\0';//Sin esto recive una "orden extra" y rompe, agregandolo, me aseguro que esa "orden extra" vaya al default para que todo siga funcionando como deberia
	return;//Retorna a selectorConexiones() apenas se haya recibido una orden desde la consola para dar lugar a las otras consolas/CPUs

}

int crearNuevoProceso(char*buffer,int size){

	printf("%d\n\n",list_size(colaListos));

	if(list_size(colaListos) >= gradoMultiProg){ /*Checkeo el grado de multiprogramacion*/
		printf("ERROR:Capacidad limite de procesos en sistema\n");
		return -1;
	}


	t_pcb* procesoListo = malloc(sizeof(t_pcb));
	procesoListo->pid = contadorPid;
	procesoListo->cantidadPaginas=1; // Es arbitrario. Hay que hacer un analisis antes de esto.

	char comandoInicializacion = 'A';
	char comandoAlmacenar = 'C';


	//Pide Memoria
	send(socketMemoria,&comandoInicializacion,sizeof(char),0); // Inicializa el handler connection de la memoria
	enviar_string(socketMemoria,buffer); // Le manda el contenido
	send(socketMemoria,&procesoListo->pid,sizeof(int),0);
	send(socketMemoria,&procesoListo->cantidadPaginas,sizeof(int),0);
	printf("Ya Inicializo programa\n");


	int offset=1; // valor arbitrario
	// Ahora pido almacenar contenido en memoria.
	send(socketMemoria,&comandoAlmacenar,sizeof(char),0); // Inicializa el handler connection de la memoria
	send(socketMemoria,&procesoListo->pid,sizeof(int),0);
	send(socketMemoria,&procesoListo->cantidadPaginas,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	enviar_string(socketMemoria,buffer);

	/*Aca se podria crear el pcb y encolarlo

		encolarProcesoListo(procesoListo); // lo encolo en ready
		una vez encolado, se lo manda a la lista consolas
		*/

	printf("Ya almacene el buffer en la memoria \n");
	return 0;
}

void encolarProcesoListo(t_pcb *pcbProcesoListo){
//	colaListos
	list_add(colaListos,pcbProcesoListo); // Agrega un pcb en la cola de listos. Al final
}

void inicializarColaListos(){
	colaListos= list_create();
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
	gradoMultiProg = atoi(config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION"));
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
	printf(	"QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%s\n",	quantum, quantumSleep, algoritmo, gradoMultiProg, semIds, semInit, sharedVars, stackSize);
	printf("---------------------------------------------------\n");

}

void escribirLog(char* mensaje){
	fwrite(mensaje,1,sizeof(mensaje),logFile);
}
void inicializarLog(){
	logFile=fopen("logKernel.txt","w");
}
