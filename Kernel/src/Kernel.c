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
#include <parser/metadata_program.h>
#include <parser/parser.h>


typedef struct{
	int pagina;
	int offset;
	int size;
}__attribute__((packed)) t_posMemoria;


typedef struct{
	char idVar;
	t_posMemoria* dirVar;
} t_variable;

typedef struct{
	t_list* args; //Lista de argumentos. Cada posicion representa un argumento en el orden de la lista
	t_list* vars; // Lista de t_variable
	int retPos;
	t_posMemoria* retVar;
} t_nodoStack;


typedef struct {
		int pid;
		t_puntero_instruccion programCounter;
		int cantidadPaginasCodigo;
		int cantidadInstrucciones;
		int** indiceCodigo;
		int cantidadEtiquetas;
		char* indiceEtiquetas;
		t_list* indiceStack;
		int exitCode;
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
int stackSize;
int paginaSize;

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
int crearNuevoProceso(char* programa,int programSize,t_pcb* procesoListo);
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

//---------PCB-----------------//
t_pcb* crearPcb (char* programa, int programSize);
int cantidadPaginasCodigoProceso(int programSize);
int calcularIndiceCodigoSize(int cantidadInstrucciones);
int calcularIndiceEtiquetasSize(int cantidadEtiquetas);
int calcularIndiceStackSize(t_list* indiceStack);
void serializarStack(char* buffer,t_list* indiceStack);
int calcularPcbSize(t_pcb* pcb);
void serializarPcbYEnviar(t_pcb* procesoAEjecutar,int socketCPU);


int** transformarIndiceCodigoSerializado(t_size cantidadInstrucciones, t_intructions* instrucciones_serializados);
int** inicializarIndiceCodigo(t_size cantidadInstrucciones);
//---------PCB-----------------//

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

				list_iterate(listaConsolas, sumarPids);
				printf("la suma de los pids es: %d", totalPids);//Para verificar si se elimino el pid deseado de la lista del kernel

				break;
		case 'P':
				send(socketAceptado,&paginaSize,sizeof(int),0);
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


int atenderNuevoPrograma(int socketAceptado){
	char* programa;
	int programSize;
		log_info(loggerConPantalla,"Se ha avisado que un archivo esta por enviarse\n");
		log_info(loggerConPantalla,"---------- Peticion de inicializar programa --------- \n");

		recv(socketAceptado,&programSize, sizeof(int),0);
		programa = malloc(programSize);
		recv(socketAceptado,programa,programSize  ,0);

		contadorPid++;
		t_pcb* procesoListo=crearPcb(programa,programSize);
		log_info(loggerConPantalla,"Program Size: %d \n", programSize);
		log_info(loggerConPantalla ,"Program Code: \" %s \" \n", programa);

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

		if((crearNuevoProceso(programa,programSize, procesoListo))<0) return -3;

		cargarConsola(procesoListo->pid,socketAceptado);

		//free(buffer);
		return 0;
}

t_pcb* crearPcb (char* programa, int programSize){
	log_info(loggerConPantalla,"Creando PCB ---> PID: %d", contadorPid);
	t_pcb* pcb = malloc (sizeof(t_pcb));
	t_metadata_program* metadata = metadata_desde_literal(programa);
	pcb->pid = contadorPid;
	pcb->programCounter = metadata->instruccion_inicio;
	pcb->cantidadPaginasCodigo = cantidadPaginasCodigoProceso(programSize);
	pcb->cantidadInstrucciones = metadata->instrucciones_size;
	pcb->indiceCodigo = transformarIndiceCodigoSerializado(pcb->cantidadInstrucciones,metadata->instrucciones_serializado);
	pcb->cantidadEtiquetas = metadata->cantidad_de_etiquetas;
	pcb->indiceEtiquetas = malloc(sizeof(char)* metadata->etiquetas_size);
	memcpy(pcb->indiceEtiquetas,metadata->etiquetas,sizeof(char)* metadata->etiquetas_size);
	pcb->indiceStack = list_create();

	metadata_destruir(metadata);
	return pcb;
}

int** transformarIndiceCodigoSerializado(t_size cantidadInstrucciones, t_intructions* instrucciones_serializados){
	int **indiceCodigo=inicializarIndiceCodigo(cantidadInstrucciones);
	log_info(loggerConPantalla, "Transformando Indice de Codigo serializado");
	int i;
		for(i = 0; i < cantidadInstrucciones; i++){
			indiceCodigo[i][0] = (instrucciones_serializados + i)->start;
			indiceCodigo[i][1] = (instrucciones_serializados + i)->offset;
		}
		return indiceCodigo;
}

int** inicializarIndiceCodigo(t_size cantidadInstrucciones){
	log_info(loggerConPantalla, "Inicializando Indice de Codigo");
	int i;
	int** indiceCodigo = malloc(sizeof(int*) * cantidadInstrucciones);
	for(i=0; i < cantidadInstrucciones; i++) indiceCodigo[i] = malloc(sizeof(int) * 2);
		return indiceCodigo;
}

void serializarPcbYEnviar(t_pcb* procesoAEjecutar,int socketCPU){
	int pcbSize = calcularPcbSize(procesoAEjecutar);
	int indiceEtiquetasSize=calcularIndiceEtiquetasSize(procesoAEjecutar->cantidadEtiquetas);
	int indiceCodigoSize=calcularIndiceCodigoSize(procesoAEjecutar->cantidadInstrucciones);
	int indiceStackSize=calcularIndiceStackSize(procesoAEjecutar->indiceStack);
	char* buffer= malloc(pcbSize);

	memcpy(buffer,&procesoAEjecutar->pid,sizeof(int));
	memcpy(buffer + sizeof(int), &procesoAEjecutar->cantidadPaginasCodigo, sizeof(int));
	memcpy(buffer + sizeof(int)*2, &procesoAEjecutar->programCounter, sizeof(t_puntero_instruccion));
	memcpy(buffer + sizeof(int)*2 + sizeof(t_puntero_instruccion), &procesoAEjecutar->cantidadInstrucciones,sizeof(int));
	memcpy(buffer + sizeof(int)*3 + sizeof(t_puntero_instruccion), &procesoAEjecutar->indiceCodigo,indiceCodigoSize);
	memcpy(buffer + sizeof(int)*3 + sizeof(t_puntero_instruccion)+ indiceCodigoSize, &procesoAEjecutar->cantidadEtiquetas,sizeof(int));
	memcpy(buffer + sizeof(int)*4 + sizeof(t_puntero_instruccion) + indiceCodigoSize , &procesoAEjecutar->indiceEtiquetas, indiceEtiquetasSize);
	serializarStack(buffer + sizeof(int)*4 + indiceCodigoSize + indiceEtiquetasSize, procesoAEjecutar->indiceStack);
	memcpy(buffer + sizeof(int)*4 + sizeof(t_puntero_instruccion) + indiceCodigoSize + indiceStackSize,&procesoAEjecutar->exitCode,sizeof(int));

	send(socketCPU,&pcbSize,sizeof(int),0);
	send(socketCPU,buffer,pcbSize,0);

}

int calcularPcbSize(t_pcb* pcb){
	return sizeof(int)*5 + sizeof(t_puntero_instruccion) + calcularIndiceCodigoSize(pcb->cantidadInstrucciones) + calcularIndiceEtiquetasSize(pcb->cantidadEtiquetas) + calcularIndiceStackSize(pcb->indiceStack);
}
int calcularIndiceStackSize(t_list* indiceStack){
	int i;
	int stackSize=0;
	t_nodoStack* node;
	for(i=0;i<indiceStack->elements_count;i++){
			node = list_get(indiceStack,i);
		stackSize+= sizeof(int) + node->args->elements_count * sizeof(t_posMemoria) + sizeof(int)+  node->vars->elements_count * sizeof(t_variable) + sizeof(int) +sizeof(t_posMemoria);
	}
	return stackSize;
}

int calcularIndiceCodigoSize(int cantidadInstrucciones){
	return cantidadInstrucciones * sizeof(int) * 2;
}

int calcularIndiceEtiquetasSize(int cantidadEtiquetas){
	return cantidadEtiquetas*sizeof(char);
}

void serializarStack(char*buffer,t_list* indiceStack){
	int i,j;
	t_nodoStack* node;

		for(i = 0; i < indiceStack->elements_count;i++){
			node = (t_nodoStack*) list_get(indiceStack, i);
			memcpy(buffer, &node->args->elements_count, sizeof(int));
			buffer += sizeof(int);

			for(j = 0; j < node->args->elements_count; j++){
				memcpy(buffer, list_get(node->args, j), sizeof(t_posMemoria));
				buffer += sizeof(t_posMemoria);
			}

			memcpy(buffer, &node->vars->elements_count, sizeof(int));
			buffer += sizeof(int);

			for(j = 0; j < node->vars->elements_count; j++){
				memcpy(buffer, list_get(node->vars,j), sizeof(t_variable));
				buffer += sizeof(t_variable);
			}

			memcpy(buffer, &node->retVar, sizeof(int));
			buffer += sizeof(int);

			memcpy(buffer, node->retVar, sizeof(t_posMemoria));
			buffer += sizeof(t_posMemoria);
		}
}


int cantidadPaginasCodigoProceso(int programSize){
	int mod = programSize % paginaSize;
	return mod==0? (programSize / paginaSize):(programSize / paginaSize)+ 1;
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

int crearNuevoProceso(char*programa,int programSize,t_pcb* procesoListo){

	if((almacenarEnMemoria(procesoListo,programa,programSize))< 0){ //TODO: Solo almaceno una pagina de codigo. Tiene que almacenar n paginas de codigo y ademas las paginas de stack
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

		//pthread_mutex_init(&mutexColaListos, NULL);
		//pthread_mutex_init(&mutexColaTerminados, NULL);
		//pthread_mutex_init(&mutexListaConsolas,NULL);

}


void inicializarListas(){
	colaNuevos= list_create();
	colaListos= list_create();
	colaTerminados= list_create();
	listaConsolas= list_create();
}

void dispatcher(int socketCPU){

	t_pcb* procesoAEjecutar = malloc(sizeof(t_pcb));

	pthread_mutex_lock(&mutexColaListos);
	procesoAEjecutar = list_get(colaListos, 0);
	list_remove(colaListos, 0);
	pthread_mutex_unlock(&mutexColaListos);

	serializarPcbYEnviar(procesoAEjecutar,socketCPU);
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

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
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
	stackSize = atoi(config_get_string_value(configuracion_kernel, "STACK_SIZE"));
	paginaSize = atoi(config_get_string_value(configuracion_kernel,"PAGINA_SIZE"));
}


