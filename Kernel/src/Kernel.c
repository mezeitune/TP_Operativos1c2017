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
#include "conexiones.h"
#include <commons/log.h>


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

char *quantum;
char *quantumSleep;
char *algoritmo;
int gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;
char *stackSize;

//--------Sincronizacion-------//

void inicializarSemaforos();
pthread_mutex_t mutexColaListos;
pthread_mutex_t mutexColaTerminados;
pthread_mutex_t mutexListaConsolas;
//------------------------------//


//--------Configuraciones--------//
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
t_config* configuracion_kernel;

//--------configuraciones-------//


//----------Planificacion----------//
int atenderNuevoPrograma(int socketAceptado);
int crearNuevoProceso(char* buffer,int size,t_pcb* procesoListo);
int verificarGradoDeMultiprogramacion();
void encolarProcesoListo(t_pcb *procesoListo);
void cargarConsola(int pid, int idConsola);
void inicializarListas();
void dispatcher(int socket);
void terminarProceso(int socket);

t_list* colaNuevos;
t_list* colaListos;
t_list* colaTerminados;
t_list* listaConsolas;
int contadorPid=0;
//---------Planificacion--------//

//--------ConnectionHandler--------//
void connectionHandler(int socketAceptado, char orden);
//---------ConnectionHandler-------//

//--------InterfazHandler--------//
void interfazHandler();
void imprimirInterfazUsuario();

void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//--------InterfazHandler-------//


//---------Conexiones---------------//
void *get_in_addr(struct sockaddr *sa);
void nuevaOrdenDeAccion(int puertoCliente, char nuevaOrden);
void selectorConexiones(int socket);
char *ipServidor;
char *ipMemoria;
char *ipFileSys;
char *puertoServidor;
char *puertoMemoria;
char *puertoFileSys;
//---------Conexiones-------------//

//------------Sockets unicos globales--------------------//
int socketMemoria;
int socketFyleSys;
int socketServidor; // Para CPUs y Consolas
//------------Sockets unicos globales-------------------//

//---------Conexion con memoria--------//
int solicitarContenidoAMemoria(char** mensajeRecibido);
int pedirMemoria(t_pcb* procesoListo);
int almacenarEnMemoria(t_pcb* procesoListoAutorizado, char* buffer, int size);
//---------Conexion con memoria--------//


int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	imprimirInterfazUsuario();

	inicializarSemaforos();

	inicializarLog("/home/utnso/Log/logKernel.txt");
	inicializarListas();

	socketServidor = crear_socket_servidor(ipServidor, puertoServidor);
	socketMemoria = crear_socket_cliente(ipMemoria, puertoMemoria);
	socketFyleSys = crear_socket_cliente(ipFileSys, puertoFileSys);

	while(1){
	/*Multiplexor de conexiones. */
		selectorConexiones(socketServidor);
	}

	return 0;
}



void interfazHandler(){

	char orden;
	int pid;
	char* mensajeRecibido;

		imprimirInterfazUsuario();
		scanf("%c",&orden);

	switch(orden){
			case 'O':
				/*obtenerListadoProgramas(); TODO HAY QUE IMPLEMENTAR*/
				break;
			case 'P':
				printf("Ingresar PID del proceso");

				scanf("%d",&pid);

				/*obtenerProcesoDato(int pid); TODO HAY QUE IMPLEMENTAR*/
				break;
			case 'G':
				/*mostrarTablaGlobalArch(); TODO HAY QUE IMPLEMENTAR*/
				break;
			case 'M':
				/*modificarGradoMultiProg(int grado) TODO HAY QUE IMPLEMETAR*/
				break;
			case 'K':
				/*finalizarProceso(int pid) TODO HAY QUE IMPLEMENTAR*/
				break;
			case 'D':
				/*pausarPlanificacion() TODO HAY QUE IMPLEMENTAR*/
				break;
			case 'S':
				if((solicitarContenidoAMemoria(&mensajeRecibido))<0){
					printf("No se pudo solicitar el contenido\n");
					break;
				}
				else{
					printf("El mensaje recibido de la Memoria es : %s\n" , mensajeRecibido);
					}
				break;
			default:
				log_warning(loggerConPantalla ,"\nOrden no reconocida\n");
				break;
	}
	return;
}

void connectionHandler(int socketAceptado, char orden) {
	int pidARecibir=0;

	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	_Bool verificarPid(t_consola* pidNuevo){
		return (pidNuevo->consolaId == socketAceptado);
	}

	void verCoincidenciaYEliminar(t_consola* p){
		list_remove_by_condition(listaConsolas, (void*)verificarPid);
	}

	switch (orden) {
		case 'I':
					atenderNuevoPrograma(socketAceptado);
					break;
		case 'N':
					dispatcher(socketAceptado);
					break;
		case 'T':
					terminarProceso(socketAceptado);
					break;
		case 'F':
				log_info(loggerConPantalla,"Se ha avisado que un proceso se quiere finalizar\n");
				recv(socketAceptado,&pidARecibir, sizeof(int),0);

				_Bool verificarPid(t_consola* pidNuevo){
					return (pidNuevo->pid== pidARecibir);
				}

				int totalPids = 0;
				void sumarPids(t_consola* p){
					totalPids += p->pid;
				}

				list_iterate(
						listaConsolas,
						sumarPids);
				printf("la suma de los pids es: %d", totalPids);//Para verificar si se elimino el pid deseado de la lista del kernel

				break;
		case 'Q':
				list_iterate(listaConsolas,
						verCoincidenciaYEliminar);
				break;
			default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
		} // END switch de la consola

	orden = '\0';
	return;//Retorna a selectorConexiones() apenas se haya recibido una orden desde la consola para dar lugar a las otras consolas/CPUs/InterfazKernel

}


int atenderNuevoPrograma(int socketAceptado){
	char* buffer;
	int bytesARecibir;
		log_info(loggerConPantalla,"Se ha avisado que un archivo esta por enviarse\n");
		log_info(loggerConPantalla,"---------- Peticion de inicializar programa --------- \n");

		recv(socketAceptado,&bytesARecibir, sizeof(int),0);
		log_info(loggerConPantalla,"Los bytes a recibir son: %d \n", bytesARecibir);
		buffer = malloc(bytesARecibir);

		recv(socketAceptado,buffer,bytesARecibir  ,0);
		log_info(loggerConPantalla ,"\nEl mensaje recibido es: \" %s \" \n", buffer);


		t_pcb* procesoListo = malloc(sizeof(t_pcb));

		contadorPid++;

		procesoListo->pid = contadorPid;
		procesoListo->cantidadPaginas=1; // TODO: Es arbitrario. Hay que hacer un analisis del buffer antes de esto.

		send(socketAceptado,&contadorPid,sizeof(int),0);
		send(socketAceptado, &socketAceptado, sizeof(int),0);

		if(verificarGradoDeMultiprogramacion() < 0){
			list_add(colaNuevos,procesoListo);
			return -1;
		}

		if((pedirMemoria(procesoListo))< 0){
			free(procesoListo);
			log_error(loggerConPantalla ,"\nNo se puede crear un nuevo proceso en el sistema");
			log_error(loggerConPantalla ,"\nMemoria no autorizo la solicitud de reserva");
			return -2; // TODO: Avisar a consola que no se puede ejecutar el programa por falta de memoria.
		}

		if((crearNuevoProceso(buffer,bytesARecibir, procesoListo))<0) return -3;

		cargarConsola(procesoListo->pid,socketAceptado);

		free(buffer);
		return 0;
}

void cargarConsola(int pid, int socketConsola) {
	t_consola *infoConsola = malloc(sizeof(t_consola));
	infoConsola->consolaId=socketConsola;
	infoConsola->pid=pid;
	pthread_mutex_lock(&mutexListaConsolas);
	list_add(listaConsolas,infoConsola);
	pthread_mutex_unlock(&mutexListaConsolas);
	free(infoConsola);
}
void enviarAImprimirALaConsola(int socketConsola, void* buffer, int size){
	void* mensajeAConsola = malloc(sizeof(int)*2 + sizeof(char));

	memcpy(mensajeAConsola,&size, sizeof(char));
	memcpy(mensajeAConsola + sizeof(int)+ sizeof(char), buffer,size);
	send(socketConsola,mensajeAConsola,sizeof(char)+ sizeof(int),0);
}

int crearNuevoProceso(char*buffer,int size,t_pcb* procesoListo){

	if((almacenarEnMemoria(procesoListo,buffer,size))< 0){
				free(procesoListo);
				log_error(loggerConPantalla ,"\nNo se puede crear un nuevo proceso en el sistema");
				log_error(loggerConPantalla ,"\nMemoria no puede almacenar contenido");
				return -1;
			}
	log_info(loggerConPantalla ,"Se inicializo el programa correctamente\n");
	log_info(loggerConPantalla ,"Se almaceno el programa correctamente\n");

	encolarProcesoListo(procesoListo);
	free(procesoListo);
	return 0;
}

int pedirMemoria(t_pcb* procesoListo){

		void* mensajeAMemoria = malloc(sizeof(int)*2 + sizeof(char));
		int resultadoEjecucion=1;
		char comandoInicializacion = 'A';

		memcpy(mensajeAMemoria,&comandoInicializacion,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char), &procesoListo->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(char) + sizeof(int) , &procesoListo->cantidadPaginas , sizeof(int));
		send(socketMemoria,mensajeAMemoria,sizeof(int)*2 + sizeof(char),0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		free(mensajeAMemoria);
		return resultadoEjecucion;
}

int almacenarEnMemoria(t_pcb* procesoListoAutorizado,char* buffer, int size){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';
		int offset=0; // valor arbitrario
		int paginaSolicitada = 0; // valor arbitrario

		void * mensajeAMemoria= malloc(sizeof(char) + sizeof(int)* 4 + size);
		memcpy(mensajeAMemoria,&comandoAlmacenar,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char),&procesoListoAutorizado->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)+sizeof(char),&paginaSolicitada,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*2 + sizeof(char),&offset,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*3 + sizeof(char),&size,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*4 + sizeof(char),buffer,size);
		send(socketMemoria,mensajeAMemoria,sizeof(char) + sizeof(int)* 4 + size,0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		return resultadoEjecucion;
		free(mensajeAMemoria);
}


int verificarGradoDeMultiprogramacion(){
	if(list_size(colaListos) >= gradoMultiProg) {
		log_error(loggerConPantalla, "Capacidad limite de procesos en sistema\n");
		return -1;
	}
	return 0;
}

int solicitarContenidoAMemoria(char ** mensajeRecibido){
	char comandoSolicitud= 'S';
	int pid;
	int paginaSolicitada;
	int offset;
	int size;
	int resultadoEjecucion;
	printf("Se inicializa una peticion de consulta\n");
	send(socketMemoria,&comandoSolicitud,sizeof(char),0);
	printf("Ingrese el pid del proceso solicitado\n");
	scanf("%d",&pid);
	printf("Ingrese la pagina solicitada\n");
	scanf("%d",&paginaSolicitada);
	printf("Ingrese el offset \n");
	scanf("%d",&offset);
	printf("Ingrese el tamano del proceso\n");
	scanf("%d",&size);

	send(socketMemoria,&pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*mensajeRecibido = recibir_string(socketMemoria);
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

void encolarProcesoListo(t_pcb *pcbProcesoListo){
//	colaListos
	pthread_mutex_lock(&mutexColaListos);
	list_add(colaListos,pcbProcesoListo);
	pthread_mutex_unlock(&mutexColaListos);
}

void inicializarSemaforos(){

		pthread_mutex_init(&mutexColaListos, NULL);
		pthread_mutex_init(&mutexColaTerminados, NULL);
		pthread_mutex_init(&mutexListaConsolas,NULL);

}


void inicializarListas(){
	colaNuevos= list_create();
	colaListos= list_create();
	colaTerminados= list_create();
	listaConsolas= list_create();
}


void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(loggerConPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(loggerConPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
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
	FD_SET(0, &master); // Agrega el fd del teclado.
	fdMax = socket; // keep track of the biggest file descriptor so far, it's this one

	// main loop
	for (;;) {
		readFds = master; // copy it
		if (select(fdMax + 1, &readFds, NULL, NULL, NULL) == -1) {
			perror("select");
			log_error(loggerSinPantalla,"Error en select\n");
			exit(2);
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdMax; i++) {
			if (FD_ISSET(i, &readFds)) { // we got one!!

				/**********************************************************/
				if(i == 0)interfazHandler();//Recibe a si mismo
				/************************************************************/

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
						log_info(loggerConPantalla,"\nSelectserver: nueva conexion desde %s en " "socket %d\n\n",inet_ntop(remoteaddr.ss_family,	get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), newfd);
					}
				}
				else if(i!=0) {
					// handle data from a client
					if ((nbytes = recv(i, &orden, sizeof orden, 0) <= 0)) { // Aca se carga el buffer con el mensaje. Actualmente no lo uso

						// got error or connection closed by client
						if (nbytes == 0) {
							// connection closed
							log_error(loggerConPantalla,"selectserver: socket %d hung up\n", i);
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
									connectionHandler(i, orden);
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
	stackSize = config_get_string_value(configuracion_kernel, "STACK_SIZE");
}



void dispatcher(int socketCPU){

	t_pcb* pcbAEnviar = malloc(sizeof(t_pcb));

	pthread_mutex_lock(&mutexColaListos);
	pcbAEnviar = list_get(colaListos, 0);
	list_remove(colaListos, 0);
	pthread_mutex_unlock(&mutexColaListos);


	send(socketCPU, pcbAEnviar, sizeof(t_pcb), 0);
}





void terminarProceso(int socketCPU){
	t_pcb* pcbProcesoTerminado = malloc(sizeof(t_pcb));
	recv(socketCPU,pcbProcesoTerminado,sizeof(t_pcb),0);

	pthread_mutex_lock(&mutexColaTerminados);
	list_add(colaTerminados,pcbProcesoTerminado);
	pthread_mutex_unlock(&mutexColaTerminados);

	/* TODO: Buscar Consola por PID e informar */

	/* TODO:Liberar recursos */

}


void imprimirConfiguraciones() {
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP FS:%s\nPUERTO FS:%s\n",ipMemoria,puertoMemoria,ipFileSys,puertoFileSys);
	printf("---------------------------------------------------\n");
	printf(	"QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%s\n",	quantum, quantumSleep, algoritmo, gradoMultiProg, semIds, semInit, sharedVars, stackSize);
	printf("---------------------------------------------------\n");

}

void imprimirInterfazUsuario(){

	/**************************************Printea interfaz Usuario Kernel*******************************************************/
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	printf("Para realizar acciones permitidas en la consola Kernel, seleccionar una de las siguientes opciones\n");
	printf("\nIngresar orden de accion:\nO - Obtener listado programas\nP - Obtener datos proceso\nG - Mostrar tabla global de archivos\nM - Modif grado multiprogramacion\nK - Finalizar proceso\nD - Pausar planificacion\n");
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	/****************************************************************************************************************************/
}

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}
