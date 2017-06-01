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
#include <commons/log.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "pcb.h"
#include "conexiones.h"
#include "interfazHandler.h"
#include "sincronizacion.h"
#include "planificacion.h"
#include "configuraciones.h"







char** tablaGlobalArchivos;
typedef struct FS{//Para poder guardar en la lista
	char** tablaArchivoPorProceso;
}t_tablaArchivoPorProceso;
t_list* listaTablasArchivosPorProceso;


void* planificarCortoPlazo();
pthread_t planificadorCortoPlazo;
void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex);


//--------ConnectionHandler--------//
void connectionHandler(int socketAceptado, char orden);
//---------ConnectionHandler-------//

//------InterruptHandler-----//
void interruptHandler(int socket,char orden);
int buscarSocketHiloPrograma(int pid);
//------InterruptHandler-----//


//---------Conexiones---------------//
void *get_in_addr(struct sockaddr *sa);
void nuevaOrdenDeAccion(int puertoCliente, char nuevaOrden);
void selectorConexiones(int socket);
void eliminarSockets(int socketConsolaGlobal,int* procesosAFinales);
fd_set master;
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
int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado, char* programa, int programSize);
int calcularTamanioParticion(int *programSizeRestante);
void handshakeMemoria();
void handshakeFS();
//---------Conexion con memoria--------//

void inicializarListas();

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	imprimirInterfazUsuario();

	inicializarSemaforos();

	inicializarLog("/home/utnso/Log/logKernel.txt");
	inicializarListas();
	inicializarSockets();
	handshakeMemoria();
	handshakeFS();
	gradoMultiProgramacion=0;
	pthread_create(&planificadorCortoPlazo, NULL,planificarCortoPlazo,NULL);
	pthread_create(&planificadorLargoPlazo, NULL,(void*)planificarLargoPlazo,NULL);

	selectorConexiones(socketServidor);

	return 0;
}

void handshakeMemoria(){
	char comandoTamanioPagina = 'P';
	send(socketMemoria,&comandoTamanioPagina,sizeof(char),0);
	recv(socketMemoria,&config_paginaSize,sizeof(int),0);
}




void handshakeFS(){
	char comandoTamanioPagina = 'V';
	char* archivoAVerificar="alumnoosdad.bin";
	int tamano=strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&comandoTamanioPagina,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	printf("Mande todoo\n");
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion es : %d\n",validado);

}











void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex){

	pthread_mutex_lock(&mutex);
	list_add(lista, elemento);
	pthread_mutex_unlock(&mutex);
}


void connectionHandler(int socketAceptado, char orden) {

	t_cpu* cpu = malloc(sizeof(t_cpu));
	int pidARecibir=0;
	char comandoEnviarPcb='S';
	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);
	_Bool verificarPid(t_consola* pidNuevo){
		return (pidNuevo->socketHiloPrograma == socketAceptado);
	}
	_Bool verificaSocket(t_cpu* cpu){
			return (cpu->socket == socketAceptado);
		}

	switch (orden) {
		case 'I':
					atenderNuevoPrograma(socketAceptado);
					break;
		case 'N':
					pthread_mutex_lock(&mutexListaCPU);
					if(!list_any_satisfy(listaCPU,(void*)verificaSocket)){
					t_cpu* cpu = malloc (sizeof(t_cpu));
					cpu->socket = socketAceptado;
					list_add(listaCPU,cpu);
					}
					pthread_mutex_unlock(&mutexListaCPU);
					send(socketAceptado,&comandoEnviarPcb,sizeof(char),0);
					sem_post(&sem_CPU);
					break;
		case 'T':
					log_info(loggerConPantalla,"\nProceso finalizado exitosamente desde CPU con socket : %d asignado",socketAceptado);
					terminarProceso(socketAceptado);
					break;
		case 'P':
				send(socketAceptado,&config_paginaSize,sizeof(int),0);
				send(socketAceptado,&stackSize,sizeof(int),0);
				break;
		case 'X':
				recv(socketAceptado,&orden,sizeof(char),0);
				interruptHandler(socketAceptado,orden);
			break;
		default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
		} // END switch de la consola

	pthread_mutex_unlock(&mutexConexion);
	orden = '\0';
	return;

}

void interruptHandler(int socketAceptado,char orden){
	log_info(loggerConPantalla,"Ejecutando interrupt handler\n");
	void* mensaje;
	int size;
	int pid;
	int socketHiloPrograma;
	int i=0;

	switch(orden){
	case 'A':
		log_info(loggerConPantalla,"Informando a Consola excepcion por problemas al reservar recursos\n");
		mensaje="El programa ANSISOP no puede iniciar actualmente debido a que no se pudo reservar recursos para ejecutar el programa, intente mas tarde";
		strcat(mensaje,"\0");
		size=strlen(mensaje);
		informarConsola(socketAceptado,mensaje,size);
		log_info(loggerConPantalla,"El programa ANSISOP enviado por socket: %d ha sido expulsado del sistema e se ha informado satifactoriamente",socketAceptado);
		break;
	case 'P':
		log_info(loggerConPantalla,"Iniciando rutina para imprimir por consola\n");
		recv(socketAceptado,&size,sizeof(int),0);
		printf("pasa1");
		mensaje=malloc(size);
		recv(socketAceptado,&mensaje,size,0);
		printf("pasa2");
		recv(socketAceptado,&pid,sizeof(int),0);

		socketHiloPrograma = buscarSocketHiloPrograma(pid);
		informarConsola(socketHiloPrograma,mensaje,size);
		log_info(loggerConPantalla,"Rutina para imprimir finalizo ----- PID: %d\n", pid);
			break;
	case 'M':
		log_info(loggerConPantalla,"Informando a Consola excepcion por grado de multiprogramacion\n");
		mensaje = "El programa ANSISOP no puede iniciar actualmente debido a grado de multiprogramacion, se mantendra en espera hasta poder iniciar";
		strcat(mensaje,"\0");
		size=strlen(mensaje);
		informarConsola(socketAceptado,mensaje,size);
		break;
	case 'C':
		log_info(loggerConPantalla,"La CPU de socket %d se ha cerrado por  signal\n",socketAceptado);
		/*TODO: Eliminar la CPU que se desconecto. De la lista de CPUS y de la lista de SOCKETS*/
		break;
	case 'E':
		log_info(loggerConPantalla,"La Consola con socket %d asignado se ha cerrado por signal \n",socketAceptado);
		recv(socketAceptado,&size,sizeof(int),0);
		mensaje = malloc(size);
		recv(socketAceptado,mensaje,size,0);
		strcpy(mensaje+size,"\0");
		int* procesosAFinalizar = malloc(size);
		for(i=0;i<size;i++){
			memcpy(procesosAFinalizar,mensaje,sizeof(int));
			mensaje += sizeof(int);
		}
		//printf("%d\n", procesosAFinalizar[0]);
		/*TODO:Hay que buscar a todos los procesosAFinalizar en la cola de los estados, y llevarlos a la cola de terminados*/

		eliminarSockets(socketAceptado,procesosAFinalizar);

		break;
	case 'F':
		log_info(loggerConPantalla,"La consola con socket: %d asignado ha solicitado finalizar un proceso ",socketAceptado);
		recv(socketAceptado,&pid,sizeof(int),0);
		log_info(loggerConPantalla,"Finalizando proceso-----PID: %d",pid);
		/*TODO> Buscar al procesoAFinalizar en la cola de los estados, llevarlo a la cola de terminados*/

		break;
	default:
			break;
	}
}

void eliminarSockets(int socketConsolaGlobal,int* procesosAFinalizar){
			close(socketConsolaGlobal);
			FD_CLR(socketConsolaGlobal,&master);
	/*TODO: Buscar por pid y borrar los socket de los hilos programas*/
}

int buscarSocketHiloPrograma(int pid){

	_Bool verificarPid(t_consola* consolathread,int pid){
		return (consolathread->pid == pid);
	}
	int socketHiloConsola;
	t_consola* consolathread = list_find(listaCodigosProgramas,(void*)verificarPid);
	socketHiloConsola =consolathread->socketHiloPrograma;
	free(consolathread);
	return socketHiloConsola;
}

void* planificarCortoPlazo(){
	t_pcb* pcbListo;
	int pid;
	t_cpu* cpuEnEjecucion = malloc(sizeof(t_cpu));



	while(1){

		sem_wait(&sem_colaReady);
		sem_wait(&sem_CPU);


		pthread_mutex_lock(&mutexColaListos);
		pcbListo = list_remove(colaListos,0);
		pthread_mutex_unlock(&mutexColaListos);

		/*La primera cpu vuelve a la segunda vuelta a la misma posicion, entonces se manda el segundo pcb a la misma cpu y estalla*/
		pthread_mutex_lock(&mutexListaCPU);
		cpuEnEjecucion = list_remove(listaCPU,0);
		cpuEnEjecucion->pid = pcbListo->pid;
		list_add(listaCPU,cpuEnEjecucion);
		pthread_mutex_unlock(&mutexListaCPU);



		pthread_mutex_lock(&mutexColaEjecucion);
		list_add(colaEjecucion, pcbListo);
		pthread_mutex_unlock(&mutexColaEjecucion);

		serializarPcbYEnviar(pcbListo, cpuEnEjecucion->socket);



	}
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

int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado,char* programa, int programSize){
	log_info(loggerConPantalla, "Almacenando programa en memoria ---- PID: %d", procesoListoAutorizado->pid);
		char* mensajeAMemoria = malloc(sizeof(char) + sizeof(int)* 4 + config_paginaSize);
		char* particionCodigo = malloc(config_paginaSize);
		int particionSize;
		int programSizeRestante = programSize;
		int resultadoEjecucion=0;
		int comandoAlmacenar = 'C';
		int offset=0;
		int nroPagina;

		log_info(loggerConPantalla, "Paginas de codigo a almacenar: %d", procesoListoAutorizado->cantidadPaginasCodigo);

		for(nroPagina=0; nroPagina<procesoListoAutorizado->cantidadPaginasCodigo && resultadoEjecucion==0;nroPagina++){
				log_info(loggerConPantalla, "Numero de pagina: %d",nroPagina);
				particionSize=calcularTamanioParticion(&programSizeRestante);
				log_info(loggerConPantalla, "Tamano de la particion de codigo a almacenar:\n %d\n", particionSize);
				strncpy(particionCodigo,programa,particionSize);
				strcpy(particionCodigo + particionSize,"\0");
				programa += particionSize;

				log_info(loggerConPantalla, "Particion de codigo a almacenar: \n%s", particionCodigo);

				memcpy(mensajeAMemoria,&comandoAlmacenar,sizeof(char));
				memcpy(mensajeAMemoria + sizeof(char),&procesoListoAutorizado->pid,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)+sizeof(char),&nroPagina,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)*2 + sizeof(char),&offset,sizeof(int));

				memcpy(mensajeAMemoria + sizeof(int)*3 + sizeof(char),&particionSize,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)*4 + sizeof(char),particionCodigo,particionSize);
				send(socketMemoria,mensajeAMemoria,sizeof(char) + sizeof(int)* 4 + particionSize,0);

				recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		}
		log_info(loggerConPantalla, "Programa almacenado en Memoria---- PID: %d", procesoListoAutorizado->pid);
		free(mensajeAMemoria);
		free(particionCodigo);

		return resultadoEjecucion;

}
int calcularTamanioParticion(int *programSizeRestante){
	log_info(loggerConPantalla,"Calculando tamano de particion de codigo");
		int mod=*programSizeRestante % config_paginaSize;
				 if(mod == *programSizeRestante){
					return *programSizeRestante;
			 }
				else{
					*programSizeRestante -= config_paginaSize;
					return config_paginaSize;
			 }
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
	*mensajeRecibido = malloc((size + 1 )*sizeof(char));
	recv(socketMemoria,*mensajeRecibido,size,0);
	strcpy(*mensajeRecibido+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

void inicializarSockets(){
		socketServidor = crear_socket_servidor(ipServidor, puertoServidor);
		socketMemoria = crear_socket_cliente(ipMemoria, puertoMemoria);
		socketFyleSys = crear_socket_cliente(ipFileSys, puertoFileSys);
}

void inicializarListas(){
	colaNuevos= list_create();
	colaListos= list_create();
	colaTerminados= list_create();
	listaConsolas = list_create();
	listaCPU = list_create();
	listaCodigosProgramas=list_create();
	listaTablasArchivosPorProceso=list_create();
	colaEjecucion = list_create();
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
	int i;        // Contador para las iteracion dentro del FDSET
	int nbytes; // El tamanio de los datos que se recibe por recv
	char orden;
	char remoteIP[INET6_ADDRSTRLEN];

	socklen_t addrlen;
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
				if(i == 0){
					interfazHandler();//Recibe a si mismo
				}
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
						log_info(loggerConPantalla,"\nSelectserver: nueva conexion desde %s en " "socket %d\n\n",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), newfd);
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
							pthread_mutex_lock(&mutexConexion);
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
