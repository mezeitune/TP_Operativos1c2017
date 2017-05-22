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
#include <commons/log.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <semaphore.h>
#include <pthread.h>
#include "pcb.h"
#include "conexiones.h"


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
int stackSize;
int paginaSize;

//--------Sincronizacion-------//

void inicializarSemaforos();
pthread_mutex_t mutexColaListos;
pthread_mutex_t mutexColaTerminados;
pthread_mutex_t mutexListaConsolas;
pthread_mutex_t mutexListaCPU;
sem_t sem_admitirNuevoProceso;
sem_t sem_colaReady;
sem_t sem_CPU;
//------------------------------//
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;

//--------Configuraciones--------//
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
t_config* configuracion_kernel;

//--------configuraciones-------//


//----------Planificacion----------//
int atenderNuevoPrograma(int socketAceptado);
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
t_list* listaCPU;

int contadorPid=0;
//---------Planificacion--------//

void* planificarCortoPlazo();
pthread_t planificadorCortoPlazo;
void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex);



//---------Planificador Largo Plazp------//
typedef struct {
	char* codigo;
	int size;
	int pid;
	int socketHiloConsola;
}t_codigoPrograma;

void* planificarLargoPlazo();
t_codigoPrograma* recibirCodigoPrograma(int socketConsola);
t_codigoPrograma* buscarCodigoDeProceso(int pid);
void crearProceso(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma);

t_list* listaCodigosProgramas;

pthread_t planificadorLargoPlazo;

//---------Planificador Largo Plazp------//

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
void inicializarSockets();
int socketMemoria;
int socketFyleSys;
int socketServidor; // Para CPUs y Consolas
//------------Sockets unicos globales-------------------//

//---------Conexion con memoria--------//
int solicitarContenidoAMemoria(char** mensajeRecibido);
int pedirMemoria(t_pcb* procesoListo);
int almacenarEnMemoria(t_pcb* procesoListoAutorizado, char* programa, int programSize);
//---------Conexion con memoria--------//


int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	imprimirInterfazUsuario();

	inicializarSemaforos();

	inicializarLog("/home/utnso/Log/logKernel.txt");
	inicializarListas();
	inicializarSockets();

	pthread_create(&planificadorCortoPlazo, NULL,planificarCortoPlazo,NULL);
	pthread_create(&planificadorLargoPlazo, NULL,planificarLargoPlazo,NULL);

	selectorConexiones(socketServidor);

	return 0;
}


void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex){

	pthread_mutex_lock(&mutex);
	list_add(lista, elemento);
	pthread_mutex_unlock(&mutex);
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
	char comandoRecibirPCB='S';
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
					send(socketAceptado,&comandoRecibirPCB,sizeof(char),0);


					agregarA(listaCPU, socketAceptado, mutexListaCPU);


					sem_post(&sem_CPU);


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

				list_iterate(listaConsolas, sumarPids);
				printf("la suma de los pids es: %d", totalPids);//Para verificar si se elimino el pid deseado de la lista del kernel

				break;
		case 'P':
				send(socketAceptado,&paginaSize,sizeof(int),0);
				send(socketAceptado,&stackSize,sizeof(int),0);
				break;
		case 'Q':
				list_iterate(listaConsolas, verCoincidenciaYEliminar);
				break;
			default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
		} // END switch de la consola

	orden = '\0';
	return;

}

t_codigoPrograma* recibirCodigoPrograma(int socketHiloConsola){
	log_info(loggerConPantalla,"Recibiendo codigo del nuevo programa ANSISOP\n");
	//char* comandoRecibirCodigo='R';
	t_codigoPrograma* codigoPrograma=malloc(sizeof(t_codigoPrograma));
	//send(socketHiloConsola,&comandoRecibirCodigo,sizeof(char),0);
	recv(socketHiloConsola,&codigoPrograma->size, sizeof(int),0);
	codigoPrograma->codigo = malloc(codigoPrograma->size);
	recv(socketHiloConsola,codigoPrograma->codigo,codigoPrograma->size  ,0);
	codigoPrograma->socketHiloConsola=socketHiloConsola;

	return codigoPrograma;
}


int atenderNuevoPrograma(int socketAceptado){
		log_info(loggerConPantalla,"Atendiendo nuevo programa\n");

		contadorPid++; // VAR GLOBAL

		t_codigoPrograma* codigoPrograma = recibirCodigoPrograma(socketAceptado);
		codigoPrograma->pid=contadorPid;


		t_pcb* proceso=crearPcb(codigoPrograma->codigo,codigoPrograma->size);
		log_info(loggerConPantalla,"Program Size: %d \n", codigoPrograma->size);
		log_info(loggerConPantalla ,"Program Code: \" %s \" \n", codigoPrograma->codigo);

		send(socketAceptado,&contadorPid,sizeof(int),0);
		//send(socketAceptado, &socketAceptado, sizeof(int),0);

		if(verificarGradoDeMultiprogramacion() <0 ){
					list_add(colaNuevos,proceso);
					list_add(listaCodigosProgramas,codigoPrograma);
					return -1; // TODO: FALTA INFORMAR A CONSOLA
				}

		crearProceso(proceso, codigoPrograma);

		return 0;
}

void crearProceso(t_pcb* proceso,t_codigoPrograma* codigoPrograma){
	if(inicializarProcesoEnMemoria(proceso,codigoPrograma) < 0 ){
				log_error(loggerConPantalla ,"\nNo se puede crear un nuevo proceso en el sistema");
				log_error(loggerConPantalla ,"\nMemoria no autorizo la solicitud de reserva");
				 // TODO: Avisar a consola que no se puede ejecutar el programa por falta de memoria y que trate mas tarde;
				// TODO: liberar la entrada en las respectivas listas;
				free(proceso);
				free(codigoPrograma);
			}

			encolarProcesoListo(proceso);
			log_info(loggerConPantalla, "PCB encolado en lista de listos ---- PID: %d", proceso->pid);
			sem_post(&sem_colaReady);//Agregado para saber si hay algo en cola Listos, es el Signal
}

int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma){
	log_info(loggerConPantalla, "Inicializando proceso en memoria ---- PID: %d \n", proceso->pid);
	if((pedirMemoria(proceso))< 0){
				free(proceso);
				free(codigoPrograma);
				log_error(loggerConPantalla ,"\nNo se puede crear un nuevo proceso en el sistema");
				log_error(loggerConPantalla ,"\nMemoria no autorizo la solicitud de reserva");
				return -1; // TODO: Avisar a consola que no se puede ejecutar el programa por falta de memoria.
			}
	log_info(loggerConPantalla ,"Existe espacio en memoria para el nuevo programa\n");

	if((almacenarEnMemoria(proceso,codigoPrograma->codigo,codigoPrograma->size))< 0){ //TODO: Solo almaceno una pagina de codigo. Tiene que almacenar n paginas de codigo y ademas las paginas de stack
					free(proceso);
					free(codigoPrograma);
					log_error(loggerConPantalla ,"\nNo se puede crear un nuevo proceso en el sistema");
					log_error(loggerConPantalla ,"\nMemoria no puede almacenar contenido");
					return -2;
				}
	log_info(loggerConPantalla ,"El nuevo programa se almaceno correctamente en memoria\n");


	return 0;
}

void* planificarLargoPlazo(int socket){
	t_pcb* proceso;
	while(1){
		sem_wait(&sem_admitirNuevoProceso); // Previamente hay que eliminar el proceso de las colas y liberar todos sus recursos.
			if(verificarGradoDeMultiprogramacion() == 0 && list_size(colaNuevos)>0){
			proceso = list_get(colaNuevos,0);
			t_codigoPrograma* codigoPrograma = buscarCodigoDeProceso(proceso->pid);
			crearProceso(proceso,codigoPrograma);
			}
	}
}


void* planificarCortoPlazo(){
	t_pcb* pcbListo;
	int socket;

	while(1){
		sem_wait(&sem_colaReady);

		pthread_mutex_lock(&mutexColaListos);
		pcbListo = list_get(colaListos,0);
		list_remove(colaListos,0);
		pthread_mutex_unlock(&mutexColaListos);

		sem_wait(&sem_CPU);

		socket = list_get(listaCPU,0);
		serializarPcbYEnviar(pcbListo, socket);
		list_remove(listaCPU,0);

	}
}


t_codigoPrograma* buscarCodigoDeProceso(int pid){
	_Bool verificarPid(t_codigoPrograma* codigoPrograma){
			return (codigoPrograma->pid == pid);
		}
	return list_find(listaCodigosProgramas, verificarPid);

}



void cargarConsola(int pid, int socketConsola) {
	t_consola *infoConsola = malloc(sizeof(t_consola));
	infoConsola->consolaId=socketConsola;
	infoConsola->pid=pid;

	pthread_mutex_lock(&mutexListaConsolas);
	list_add(listaConsolas,infoConsola);
	pthread_mutex_unlock(&mutexListaConsolas);
	//free(infoConsola);
}
void enviarAImprimirALaConsola(int socketConsola, void* buffer, int size){
	void* mensajeAConsola = malloc(sizeof(int)*2 + sizeof(char));

	memcpy(mensajeAConsola,&size, sizeof(char));
	memcpy(mensajeAConsola + sizeof(int)+ sizeof(char), buffer,size);
	send(socketConsola,mensajeAConsola,sizeof(char)+ sizeof(int),0);
}



int pedirMemoria(t_pcb* procesoListo){
	log_info(loggerConPantalla, "Solicitando Memoria ---- PID: %d", procesoListo->pid);
		void* mensajeAMemoria = malloc(sizeof(int)*2 + sizeof(char));
		int paginasTotalesRequeridas = procesoListo->cantidadPaginasCodigo + stackSize;
		int resultadoEjecucion=1;
		char comandoInicializacion = 'A';

		memcpy(mensajeAMemoria,&comandoInicializacion,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char), &procesoListo->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(char) + sizeof(int) , &paginasTotalesRequeridas, sizeof(int));
		send(socketMemoria,mensajeAMemoria,sizeof(int)*2 + sizeof(char),0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		free(mensajeAMemoria);
		return resultadoEjecucion;
}

int almacenarEnMemoria(t_pcb* procesoListoAutorizado,char* programa, int programSize){
	log_info(loggerConPantalla, "Almacenando programa en memoria ---- PID: %d", procesoListoAutorizado->pid);
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';
		int offset=0; // valor arbitrario
		int paginaPrograma = 0; // Quiero guardar los datos nomas.

		void * mensajeAMemoria= malloc(sizeof(char) + sizeof(int)* 4 + programSize);
		memcpy(mensajeAMemoria,&comandoAlmacenar,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char),&procesoListoAutorizado->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)+sizeof(char),&paginaPrograma,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*2 + sizeof(char),&offset,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*3 + sizeof(char),&programSize,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int)*4 + sizeof(char),programa,programSize);
		send(socketMemoria,mensajeAMemoria,sizeof(char) + sizeof(int)* 4 + programSize,0);

		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		free(mensajeAMemoria);
		return resultadoEjecucion;
}


int verificarGradoDeMultiprogramacion(){
	log_info(loggerConPantalla, "Verificando grado de multiprogramacion");
	pthread_mutex_lock(&mutexColaListos);
	if(list_size(colaListos) >= gradoMultiProg) {
		pthread_mutex_unlock(&mutexColaListos);
		log_error(loggerConPantalla, "Capacidad limite de procesos en sistema\n");
		return -1;
	}
	pthread_mutex_unlock(&mutexColaListos);
	log_info(loggerConPantalla, "Grado de multiprogramacion suficiente\n");

	return 0;
}

int solicitarContenidoAMemoria(char ** mensajeRecibido){
	log_info(loggerConPantalla, "Solicitando contenido a Memoria");
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

void encolarProcesoListo(t_pcb *procesoListo){
//	colaListos
	log_info(loggerConPantalla, "Encolando proceso a cola de listos---- PID: %d \n", procesoListo->pid);

	pthread_mutex_lock(&mutexColaListos);
	list_add(colaListos,procesoListo);
	pthread_mutex_unlock(&mutexColaListos);
}

void inicializarSemaforos(){

		pthread_mutex_init(&mutexColaListos, NULL);
		pthread_mutex_init(&mutexColaTerminados, NULL);
		pthread_mutex_init(&mutexListaConsolas,NULL);
		pthread_mutex_init(&mutexListaCPU,NULL);
		sem_init(&sem_admitirNuevoProceso, 0, 0);
		sem_init(&sem_colaReady,0,0);
		sem_init(&sem_CPU,0,0);
}


void inicializarListas(){
	colaNuevos= list_create();
	colaListos= list_create();
	colaTerminados= list_create();
	listaConsolas = list_create();
	listaCPU = list_create();
	listaCodigosProgramas=list_create();
}

/*void dispatcher(int socketCPU){ YA NO SE USA

	log_info(loggerConPantalla, "Despachando PCB listo ---- SOCKET:%d", socketCPU);
	t_pcb* procesoAEjecutar = malloc(sizeof(t_pcb));

	pthread_mutex_lock(&mutexColaListos);
	procesoAEjecutar = list_get(colaListos, 0);
	list_remove(colaListos, 0);
	pthread_mutex_unlock(&mutexColaListos);

	serializarPcbYEnviar(procesoAEjecutar,socketCPU);

	}
*/
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
	printf(	"QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%d\nPAGINA_SIZE:%d\n",	quantum, quantumSleep, algoritmo, gradoMultiProg, semIds, semInit, sharedVars, stackSize, paginaSize);
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


void inicializarSockets(){
		socketServidor = crear_socket_servidor(ipServidor, puertoServidor);
		socketMemoria = crear_socket_cliente(ipMemoria, puertoMemoria);
		socketFyleSys = crear_socket_cliente(ipFileSys, puertoFileSys);

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

	for (;;) {
		readFds = master;
		if (select(fdMax + 1, &readFds, NULL, NULL, NULL) == -1) {
			perror("select");
			log_error(loggerSinPantalla,"Error en select\n");
			exit(2);
		}

		for (i = 0; i <= fdMax; i++) {
			if (FD_ISSET(i, &readFds)) { // we got one!!

				/**********************************************************/
				if(i == 0)interfazHandler();//Recibe a si mismo
				/************************************************************/

				if (i == socket) {
					addrlen = sizeof remoteaddr;
					newfd = accept(socket, (struct sockaddr *) &remoteaddr,&addrlen);

					if (newfd == -1) {
						perror("accept");
					} else {
						FD_SET(newfd, &master);
						if (newfd > fdMax) {
							fdMax = newfd;
						}
						log_info(loggerConPantalla,"\nSelectserver: nueva conexion desde %s en " "socket %d\n\n",inet_ntop(remoteaddr.ss_family,	get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), newfd);
					}
				}
				else if(i!=0) {
					if ((nbytes = recv(i, &orden, sizeof orden, 0) <= 0)) { // Aca se carga el buffer con el mensaje. Actualmente no lo uso

						//if (nbytes == 0) {
						//	log_error(loggerConPantalla,"selectserver: socket %d hung up\n", i);
						//} else {
						//	perror("recv");
						//}
						//close(i);
						//FD_CLR(i, &master);
					}
					else {
						/*
						for(j = 0; j <= fdMax; j++) {//Rota entre las conexiones
							if (FD_ISSET(j, &master)) {
								if (j != socket && j != i) {*/
									connectionHandler(i, orden);
						        }
						    }
						}
					}
				}
			} // END handle data from client
		//} // END got new incoming connection
	//} // END looping through file descriptors
//}

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
	stackSize = atoi(config_get_string_value(configuracion_kernel, "STACK_SIZE"));
	paginaSize = atoi(config_get_string_value(configuracion_kernel,"PAGINA_SIZE"));
}
void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}
