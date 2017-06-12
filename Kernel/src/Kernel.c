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
#include <signal.h>
#include "pcb.h"
#include "conexiones.h"
#include "interfazHandler.h"
#include "sincronizacion.h"
#include "planificacion.h"
#include "configuraciones.h"
#include "conexionMemoria.h"
#include "capaFS.h"
#include "contabilidad.h"


void recibirPidDeCpu(int socket);

//--------ConnectionHandler--------//
void connectionHandler(int socketAceptado, char orden);
void inicializarListas();
int atenderNuevoPrograma(int socketAceptado);
t_codigoPrograma* recibirCodigoPrograma(int socketHiloConsola);
void gestionarNuevaCPU(int socketCPU);
void handShakeCPU(int socketCPU);
//---------ConnectionHandler-------//

//------InterruptHandler-----//
void interruptHandler(int socket,char orden);
void imprimirPorConsola(int socketAceptado);
int buscarSocketHiloPrograma(int pid);
void buscarProcesoYTerminarlo(int pid);
void excepcionReservaRecursos(socketAceptado);
void excepcionPlanificacionDetenida(int socket);
void gestionarCierreConsola(int socket);
//------InterruptHandler-----//


//---------Conexiones---------------//
void nuevaOrdenDeAccion(int puertoCliente, char nuevaOrden);
void selectorConexiones();
fd_set master;
int flagFinalizarKernel = 0;
//---------Conexiones-------------//


//----------shared vars-----------//
void obtenerValorDeSharedVar(int socket);
int* variablesGlobales;
int tamanioArray(char** array);
pthread_mutex_t mutexVariablesGlobales = PTHREAD_MUTEX_INITIALIZER;
int indiceEnArray(char** array, char* elemento);
void obtenerVariablesCompartidasDeLaConfig();
void guardarValorDeSharedVar(int socket);
//----------shared vars-----------//

//-------semaforos ANSISOP-------//
typedef struct{
	int valor;
	char * id;
}t_semaforo;
t_semaforo* semaforosGlobales;
void obtenerSemaforosANSISOPDeLasConfigs();
void waitSemaforoAnsisop(int socketAceptado);
void signalSemaforoAnsisop(int socketAceptado);
//-------semaforos ANSISOP-------//

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	imprimirInterfazUsuario();
	inicializarSockets();
	inicializarSemaforos();

	inicializarLog("/home/utnso/Log/logKernel.txt");
	inicializarListas();
	handshakeMemoria();

	obtenerVariablesCompartidasDeLaConfig();
	obtenerSemaforosANSISOPDeLasConfigs();

	pthread_create(&planificadorCortoPlazo, NULL,planificarCortoPlazo,NULL);
	pthread_create(&planificadorLargoPlazo, NULL,(void*)planificarLargoPlazo,NULL);
	pthread_create(&interfaz, NULL,(void*)interfazHandler,NULL);


	selectorConexiones(socketServidor);

	return 0;
}



void connectionHandler(int socketAceptado, char orden) {

	int cantidadDeRafagas;
	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	_Bool verificarPid(t_consola* pidNuevo){
		return (pidNuevo->socketHiloPrograma == socketAceptado);
	}
	_Bool verificaSocket(t_cpu* unaCpu){
		return (unaCpu->socket == socketAceptado);
	}

	int quantum = 0; //SI ES 0 ES FIFO SINO ES UN QUANTUM
	t_pcb* pcb;
	t_cpu *cpu = malloc(sizeof(t_cpu));
	char comandoDesdeCPU;

	switch (orden) {
		case 'I':
					atenderNuevoPrograma(socketAceptado);
					break;
		case 'N':

					if(!strcmp(config_algoritmo, "RR")) quantum = config_quantum;
					send(socketAceptado,&quantum,sizeof(int),0);
					gestionarNuevaCPU(socketAceptado);

					break;
		case 'T':
					terminarProceso(socketAceptado);
					break;
		case 'F'://Para el FS
					recv(socketAceptado,&comandoDesdeCPU,sizeof(char),0);
					interfazHandlerParaFileSystem(comandoDesdeCPU,socketAceptado);//En vez de la V , poner el recv de la orden que quieras hacer con FS
					break;
		case 'P':
					handShakeCPU(socketAceptado);
					break;
		case 'X':
					recv(socketAceptado,&orden,sizeof(char),0);
					interruptHandler(socketAceptado,orden);
			break;
		case 'R':
					recv(socketAceptado,&cantidadDeRafagas,sizeof(int),0);

					pthread_mutex_lock(&mutexListaCPU);
					cpu = list_remove_by_condition(listaCPU, (void*)verificaSocket);
					cpu->enEjecucion = 0;
					list_add(listaCPU,cpu);
					pthread_mutex_unlock(&mutexListaCPU);


					pcb = recibirYDeserializarPcb(socketAceptado);
					actualizarRafagas(pcb->pid,cantidadDeRafagas);
					agregarAFinQuantum(pcb);

					sem_post(&sem_CPU);
					break;
		case 'K':
					recibirPidDeCpu(socketAceptado);
					break;
		case 'S':
					obtenerValorDeSharedVar(socketAceptado);
					break;
		case 'G':
					guardarValorDeSharedVar(socketAceptado);
					break;
		case 'W':
					waitSemaforoAnsisop(socketAceptado);
					break;
		case 'L':
					signalSemaforoAnsisop(socketAceptado);
					break;
		default:
					if(orden == '\0') break;
					log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
					break;
		}

	orden = '\0';
	return;

}

int atenderNuevoPrograma(int socketAceptado){
		log_info(loggerConPantalla,"Atendiendo nuevo programa");

		contadorPid++;
		send(socketAceptado,&contadorPid,sizeof(int),0);

		t_codigoPrograma* codigoPrograma = recibirCodigoPrograma(socketAceptado);
		t_pcb* proceso=crearPcb(codigoPrograma->codigo,codigoPrograma->size);
		codigoPrograma->pid=proceso->pid;

		cargarConsola(proceso->pid,codigoPrograma->socketHiloConsola);
		log_info(loggerConPantalla,"Pcb encolado en Nuevos--->PID: %d",proceso->pid);

		if(!flagPlanificacion) {
					contadorPid--;
					free(proceso);
					log_warning(loggerConPantalla,"La planificacion del sistema esta detenida");
					interruptHandler(codigoPrograma->socketHiloConsola,'B'); // Informa a consola error por planificacion detenida
					free(codigoPrograma);
					return -1;
						}

		pthread_mutex_lock(&mutexColaNuevos);
		list_add(colaNuevos,proceso);
		pthread_mutex_unlock(&mutexColaNuevos);

		pthread_mutex_lock(&mutexListaCodigo);
		list_add(listaCodigosProgramas,codigoPrograma);
		pthread_mutex_unlock(&mutexListaCodigo);

		sem_post(&sem_admitirNuevoProceso);

		return 0;
}
t_codigoPrograma* recibirCodigoPrograma(int socketHiloConsola){
	log_info(loggerConPantalla,"Recibiendo codigo del nuevo programa ANSISOP");
	t_codigoPrograma* codigoPrograma=malloc(sizeof(t_codigoPrograma));
	recv(socketHiloConsola,&codigoPrograma->size, sizeof(int),0);
	codigoPrograma->codigo = malloc(codigoPrograma->size);
	recv(socketHiloConsola,codigoPrograma->codigo,codigoPrograma->size,0);
	strcpy(codigoPrograma->codigo + codigoPrograma->size , "\0");
	codigoPrograma->socketHiloConsola=socketHiloConsola;
	return codigoPrograma;
}

void handShakeCPU(int socketCPU){
	send(socketCPU,&config_paginaSize,sizeof(int),0);
	send(socketCPU,&stackSize,sizeof(int),0);
}

void gestionarNuevaCPU(int socketCPU){
	t_cpu* cpu = malloc(sizeof(t_cpu));
	cpu->socket = socketCPU;
	cpu->enEjecucion = 0;

	pthread_mutex_lock(&mutexListaCPU);
	list_add(listaCPU,cpu);
	pthread_mutex_unlock(&mutexListaCPU);
	sem_post(&sem_CPU);
}

void recibirPidDeCpu(int socket){
int pid;
	recv(socket,&pid,sizeof(int),0);
}
void interruptHandler(int socketAceptado,char orden){
	log_info(loggerConPantalla,"\nEjecutando interrupt handler");
	int pid;

	switch(orden){

		case 'A':
			excepcionReservaRecursos(socketAceptado);
			break;
		case 'B':
			excepcionPlanificacionDetenida(socketAceptado);
			break;
		case 'P':
			imprimirPorConsola(socketAceptado);
			break;
		case 'C':
			log_warning(loggerConPantalla,"\nLa CPU  %d se ha cerrado",socketAceptado);
			/*TODO: NACHO ---> Eliminar la CPU que se desconecto. De la lista de CPUS y de la lista de SOCKETS*/
			break;
		case 'E':
			gestionarCierreConsola(socketAceptado);
			break;
		case 'F':
			log_warning(loggerConPantalla,"\nLa consola  %d  ha solicitado finalizar un proceso ",socketAceptado);
			recv(socketAceptado,&pid,sizeof(int),0);
			finalizarProcesoVoluntariamente(pid);
			break;
		case  'R':

			//recv(socketAceptado,&size,sizeof(int),0);
			//recv(socketAceptado,&espacioAReservar,size,0);
			//reservar heap de ese espacio
			//devolverle a la CPU el puntero que apunta a donde esta reservado ese espacio en heap
			//creo que se tiene que guardar ese int del espacio para cuando se pida liberar
			break;
		case  'L':

			//recv(socketAceptado,&size,sizeof(int),0);
			//recv(socketAceptado,&punteroQueApuntaDondeLiberar,size,0);
			//liberar heap tomando como inicio ese puntero y el espacio dado anteriormente por "reservar"
			//devolverle a la CPU el resultado de la ejecucion (1 piola, 0 fuck)
			break;
		default:
			break;
	}
}

void excepcionReservaRecursos(int socket){
	char* mensaje;
	int size;
	log_info(loggerConPantalla,"Informando a Consola excepcion por problemas al reservar recursos\n");
	mensaje="El programa ANSISOP no puede iniciar actualmente debido a que no se pudo reservar recursos para ejecutar el programa, intente mas tarde";
	size=strlen(mensaje);
	informarConsola(socket,mensaje,size);
	log_info(loggerConPantalla,"El programa ANSISOP enviado por socket: %d ha sido expulsado del sistema e se ha informado satifactoriamente",socket);
}

void excepcionPlanificacionDetenida(int socket){
	char* mensaje;
	int size;
	log_info(loggerConPantalla,"Informando a Consola excepcion por planificacion detenido\n");
	mensaje = "El programa ANSISOP no puede iniciar actualmente debido a que la planificacion del sistema se encuentra detenido";
	size=strlen(mensaje);
	informarConsola(socket,mensaje,size);
	mensaje = "Finalizar";
	size=strlen(mensaje);
	informarConsola(socket,mensaje,size);
	recv(socket,&size,sizeof(int),0); // A modo de ok
	eliminarSocket(socket);
}

void imprimirPorConsola(socketAceptado){
	char* mensaje;
	int size;
	recv(socketAceptado,&size,sizeof(int),0);
	mensaje=malloc(size);
	recv(socketAceptado,&mensaje,size,0);
	recv(socketAceptado,&pid,sizeof(int),0);
	log_info(loggerConPantalla,"Imprimiendo por consola--->PID:%d",pid);
	informarConsola(buscarSocketHiloPrograma(pid),mensaje,size);
	free(mensaje);
}

void gestionarCierreConsola(int socket){
	log_warning(loggerConPantalla,"\nLa Consola %d se ha cerrado",socket);
	pthread_mutex_lock(&mutexNuevoProceso);
	log_info(loggerConPantalla,"Gestionando cierre de consola %d",socket);
	int size, cantidad,pid;
	char* procesosAFinalizar;
	int desplazamiento=0;
	int i;

		recv(socket,&size,sizeof(int),0);
		procesosAFinalizar = malloc(size);
		recv(socket,procesosAFinalizar,size,0);
		cantidad = *((int*)procesosAFinalizar+desplazamiento);
		desplazamiento++;

			for(i=0;i<cantidad;i++){
				pid = *((int*)procesosAFinalizar+desplazamiento);
						buscarProcesoYTerminarlo(pid); /*TODO: Finalizar procesos en ejecucion*/
						finalizarHiloPrograma(pid);
						desplazamiento ++;
			}

			send(socket,&i,sizeof(int),0); /*HACE ESPERAR AL HILO PROCESO EN CONSOLA*/
			eliminarSocket(socket);
			free(procesosAFinalizar);
			pthread_mutex_unlock(&mutexNuevoProceso);
}

void eliminarSocket(int socket){
	pthread_mutex_lock(&mutex_masterSet);
	FD_CLR(socket,&master);
	pthread_mutex_unlock(&mutex_masterSet);
	log_info(loggerConPantalla,"Socket %d cerrado",socket);
	close(socket);
}

void buscarProcesoYTerminarlo(int pid){
	log_info(loggerConPantalla,"Finalizando proceso--->PID: %d ",pid);

	t_pcb *procesoATerminar;
	t_cpu *cpuAFinalizar = malloc(sizeof(t_cpu));

	char comandoFinalizar = 'F';

	_Bool verificarPid(t_pcb* pcb){
			return (pcb->pid==pid);
		}
	_Bool verificarPidCPU(t_cpu* cpu){
				return (cpu->pid==pid);
			}

	pthread_mutex_lock(&mutexColaEjecucion);
		if(list_any_satisfy(colaEjecucion,(void*)verificarPid)){

			pthread_mutex_lock(&mutexListaCPU);
			if(list_any_satisfy(listaCPU,(void*)verificarPidCPU)) cpuAFinalizar = list_find(listaCPU, (void*) verificarPidCPU);
			pthread_mutex_unlock(&mutexListaCPU);

			send(cpuAFinalizar->socket, &comandoFinalizar,sizeof(char),0);
			return;
		}
		pthread_mutex_unlock(&mutexColaEjecucion);

	pthread_mutex_lock(&mutexColaNuevos);
	if(list_any_satisfy(colaNuevos,(void*)verificarPid)){
		procesoATerminar=list_remove_by_condition(colaNuevos,(void*)verificarPid);
	}
	pthread_mutex_unlock(&mutexColaNuevos);


	pthread_mutex_lock(&mutexColaListos);

	if(list_any_satisfy(colaListos,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaListos,(void*)verificarPid);
			liberarRecursosEnMemoria(procesoATerminar);
			sem_wait(&sem_colaReady);
		}
	pthread_mutex_unlock(&mutexColaListos);

	pthread_mutex_lock(&mutexColaBloqueados);
	if(list_any_satisfy(colaBloqueados,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaBloqueados,(void*)verificarPid);
			liberarRecursosEnMemoria(procesoATerminar); /*TODO: Tambien habria que limpiarlo de la cola del semaforo asociado*/
		}
	pthread_mutex_unlock(&mutexColaBloqueados);

	pthread_mutex_lock(&mutexColaTerminados);
	list_add(colaTerminados,procesoATerminar);
	pthread_mutex_unlock(&mutexColaTerminados);
}


int buscarSocketHiloPrograma(int pid){

	_Bool verificarPid(t_consola* consolathread){
		return (consolathread->pid == pid);
	}
	int socketHiloConsola;
	t_consola* consolathread = list_find(listaConsolas,(void*)verificarPid);
	socketHiloConsola =consolathread->socketHiloPrograma;
	/*free(consolathread);*/ //TODO: Ver si aca hay que liberar o no
	return socketHiloConsola;
}

void enviarAImprimirALaConsola(int socketConsola, void* buffer, int size){
	void* mensajeAConsola = malloc(sizeof(int)*2 + sizeof(char));

	memcpy(mensajeAConsola,&size, sizeof(char));
	memcpy(mensajeAConsola + sizeof(int)+ sizeof(char), buffer,size);
	send(socketConsola,mensajeAConsola,sizeof(char)+ sizeof(int),0);
}




void inicializarListas(){
	colaNuevos= list_create();
	colaListos= list_create();
	colaTerminados= list_create();
	colaEjecucion = list_create();
	colaBloqueados=list_create();

	listaConsolas = list_create();
	listaCPU = list_create();
	listaCodigosProgramas=list_create();
	listaTablasArchivosPorProceso=list_create();
	listaFinQuantum = list_create();
	listaEnEspera = list_create();

	listaContable=list_create();
}
void obtenerVariablesCompartidasDeLaConfig(){
	int tamanio = tamanioArray(shared_vars);
	int i;
	variablesGlobales = malloc(sizeof(int) * tamanio);
	for(i = 0; i < tamanio; i++) {

		variablesGlobales[i] = 0;

	}
}
void obtenerValorDeSharedVar(int socket){
	int tamanio;
	char* identificador;
	recv(socket,&tamanio,sizeof(int),0);
	identificador = malloc(tamanio);
	recv(socket,identificador,tamanio,0);
	log_info(loggerConPantalla, "Obteniendo el Valor de id: %s", identificador);
	int indice = indiceEnArray(shared_vars, identificador);
	int valor;
	pthread_mutex_lock(&mutexVariablesGlobales);
	valor = variablesGlobales[indice];
	pthread_mutex_unlock(&mutexVariablesGlobales);
	log_info(loggerConPantalla, "Valor obtenido: %d", valor);
	send(socket,&valor,sizeof(int),0);
}

void guardarValorDeSharedVar(int socket){
	int tamanio, valorAGuardar;
	char* identificador;
	recv(socket,&tamanio,sizeof(int),0);
	identificador = malloc(tamanio);
	recv(socket,identificador,tamanio,0);
	recv(socket,&valorAGuardar,sizeof(int),0);
	log_info(loggerConPantalla, "Guardar Valor de : id: %s, valor: %d", identificador, valorAGuardar);
	int indice = indiceEnArray(shared_vars, identificador);
	pthread_mutex_lock(&mutexVariablesGlobales);
	variablesGlobales[indice] = valorAGuardar;
	pthread_mutex_unlock(&mutexVariablesGlobales);
}
void obtenerSemaforosANSISOPDeLasConfigs(){
	int i, tamanio = tamanioArray(semId);
	semaforosGlobales = malloc(sizeof(t_semaforo) * tamanio);
	for(i = 0; i < tamanio; i++){
		char *ptr;
		semaforosGlobales[i].valor = strtol(semInit[i], &ptr, 10);
		semaforosGlobales[i].id = semId[i];
		}
}
void waitSemaforoAnsisop(int socketAceptado){
	int tamanio;
	char* semaforoAConsultar;
	recv(socketAceptado,&tamanio,sizeof(int),0);
	semaforoAConsultar = malloc(tamanio);
	recv(socketAceptado,semaforoAConsultar,tamanio,0);
	int bloquearScriptONo=-1;
	log_info(loggerConPantalla, "Esperar: id: %s", semaforoAConsultar);
	int indice = indiceEnArray(semId, semaforoAConsultar);
	log_info(loggerConPantalla, "Indice encontrado: %d", indice);
	log_info(loggerConPantalla, "Valor en el indice: %d", semaforosGlobales[indice]);
	send (socketAceptado,&bloquearScriptONo,sizeof(int),0);
}
void signalSemaforoAnsisop(int socketAceptado){
	int tamanio;
	char* semaforoAConsultar;
	recv(socketAceptado,&tamanio,sizeof(int),0);
	semaforoAConsultar = malloc(tamanio);
	recv(socketAceptado,semaforoAConsultar,tamanio,0);
	log_info(loggerConPantalla, "Signal: id: %s", semaforoAConsultar);
	int indice = indiceEnArray(semId, semaforoAConsultar);
	log_info(loggerConPantalla, "Indice encontrado: %d", indice);
	log_info(loggerConPantalla, "Valor en el indice: %d", semaforosGlobales[indice]);
}

int indiceEnArray(char** array, char* elemento){
	int i = 0;
	while(array[i] && strcmp(array[i], elemento)) i++;

	return array[i] ? i:-1;
}
int tamanioArray(char** array){
	int i = 0;
	while(array[i]) i++;
	return i;
}

void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(loggerConPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(loggerConPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
}
void selectorConexiones() {
	log_info(loggerConPantalla,"Iniciando selector de conexiones");
	int maximoFD;
	int nuevoFD;
	int socket;
	char orden;

	char remoteIP[INET6_ADDRSTRLEN];
	socklen_t addrlen;
	fd_set readFds;
	struct sockaddr_storage remoteaddr;// temp file descriptor list for select()


	if (listen(socketServidor, 15) == -1) {
	perror("listen");
	exit(1);
	}

	pthread_mutex_lock(&mutex_masterSet);
	FD_SET(socketServidor, &master); // add the listener to the master set
	pthread_mutex_unlock(&mutex_masterSet);

	maximoFD = socketServidor; // keep track of the biggest file descriptor so far, it's this one

	while(!flagFinalizarKernel) {
					pthread_mutex_lock(&mutex_masterSet);
					readFds = master;
					pthread_mutex_unlock(&mutex_masterSet);

					if (select(maximoFD + 1, &readFds, NULL, NULL, NULL) == -1) {
						perror("select");
						log_error(loggerSinPantalla,"Error en select\n");
						exit(2);
					}

					for (socket = 0; socket <= maximoFD; socket++) {
							if (FD_ISSET(socket, &readFds)) { //Hubo una conexion

									if (socket == socketServidor) {
										addrlen = sizeof remoteaddr;
										nuevoFD = accept(socketServidor, (struct sockaddr *) &remoteaddr,&addrlen);

										if (nuevoFD == -1) perror("accept");

										else {
											pthread_mutex_lock(&mutex_masterSet);
											FD_SET(nuevoFD, &master);
											pthread_mutex_unlock(&mutex_masterSet);

											if (nuevoFD > maximoFD)	maximoFD = nuevoFD;

											log_info(loggerConPantalla,"\nSelectserver: nueva conexion en IP: %s en socket %d",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), nuevoFD);
										}
									}
									else if(socket != 0) {
											recv(socket, &orden, sizeof(char), MSG_WAITALL);
											connectionHandler(socket, orden);
									}
							}
					}
		}
	log_info(loggerConPantalla,"Finalizando selector de conexiones");
}

