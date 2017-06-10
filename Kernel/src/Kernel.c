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
void gestionarNuevaCPU(int socketCPU);
void handShakeCPU(int socketCPU);
//---------ConnectionHandler-------//

//------InterruptHandler-----//
void interruptHandler(int socket,char orden);
int buscarSocketHiloPrograma(int pid);
void buscarProcesoYTerminarlo(int pid);
void eliminarSocket(int socket);
void gestionarCierreConsola(int socket);
void cerrarHilosProgramas(char* procesosAFinalizar,int cantidad);
//------InterruptHandler-----//


//---------Conexiones---------------//
void *get_in_addr(struct sockaddr *sa);
void nuevaOrdenDeAccion(int puertoCliente, char nuevaOrden);
void selectorConexiones();
fd_set master;
int flagFinalizarKernel = 0;
//---------Conexiones-------------//

int cantidadDeRafagas;

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
	//gradoMultiProgramacion=0;
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


	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	_Bool verificarPid(t_consola* pidNuevo){
		return (pidNuevo->socketHiloPrograma == socketAceptado);
	}
	_Bool verificaSocket(t_cpu* unaCpu){
		return (unaCpu->socket == socketAceptado);
	}


	t_pcb* pcb;

	char comandoDesdeCPU;

	switch (orden) {
		case 'I':
					atenderNuevoPrograma(socketAceptado);
					break;
		case 'N':
					gestionarNuevaCPU(socketAceptado);
					break;
		case 'T':
					recv(socketAceptado,&cantidadDeRafagas,sizeof(int),0);
					log_info(loggerConPantalla,"\nProceso finalizado exitosamente desde CPU con socket : %d asignado",socketAceptado);

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
					pcb = recibirYDeserializarPcb(socketAceptado);
					actualizarRafagas(pcb->pid,cantidadDeRafagas);
					agregarAFinQuantum(pcb);
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

		contadorPid++; // VAR GLOBAL
		send(socketAceptado,&contadorPid,sizeof(int),0);

		t_codigoPrograma* codigoPrograma = recibirCodigoPrograma(socketAceptado);
		t_pcb* proceso=crearPcb(codigoPrograma->codigo,codigoPrograma->size);
		codigoPrograma->pid=proceso->pid;

		pthread_mutex_lock(&mutexNuevoProceso);
		if(verificarGradoDeMultiprogramacion() != 0){
			log_error(loggerConPantalla,"Grado de multiprogramacion insuficiente");
			interruptHandler(socketAceptado,'M');
		}
		pthread_mutex_unlock(&mutexNuevoProceso);


		if(!flagPlanificacion) {
					contadorPid--;
					free(proceso);
					free(codigoPrograma);
					log_warning(loggerConPantalla,"La planificacion del sistema esta detenida");
					interruptHandler(socketAceptado,'D'); // Informa a consola error por planificacion detenida
					return -1;
						}

		pthread_mutex_lock(&mutexColaNuevos);
		list_add(colaNuevos,proceso);
		pthread_mutex_unlock(&mutexColaNuevos);

		pthread_mutex_lock(&mutexListaCodigo);
		list_add(listaCodigosProgramas,codigoPrograma);
		pthread_mutex_unlock(&mutexListaCodigo);

		cargarConsola(proceso->pid,codigoPrograma->socketHiloConsola);
		log_info(loggerConPantalla,"Pcb encolado en Nuevos--->PID: %d",proceso->pid);

		sem_post(&sem_admitirNuevoProceso);



		return 0;
}

void handShakeCPU(int socketCPU){
	send(socketCPU,&config_paginaSize,sizeof(int),0);
	send(socketCPU,&stackSize,sizeof(int),0);
}

void gestionarNuevaCPU(int socketCPU){
	t_cpu* cpu = malloc(sizeof(t_cpu));
	cpu->socket = socketCPU;
	list_add(listaCPU,cpu);
	sem_post(&sem_CPU);
}

void recibirPidDeCpu(int socket){
int pid;
	recv(socket,&pid,sizeof(int),0);
}
void interruptHandler(int socketAceptado,char orden){

	log_info(loggerConPantalla,"Ejecutando interrupt handler\n");

	printf("\n\n\nROMPO ACA\n\n\nY la orden es: %c\n\n\n", orden);

	int size;
	int pid;
	int socketHiloPrograma;
	char* mensaje; /*TODO: Ver el tema de pedir memoria y liberar esta variable siempre que se pueda*/

	switch(orden){

	case 'A':
		log_info(loggerConPantalla,"Informando a Consola excepcion por problemas al reservar recursos\n");
		mensaje="El programa ANSISOP no puede iniciar actualmente debido a que no se pudo reservar recursos para ejecutar el programa, intente mas tarde";
		size=strlen(mensaje);
		informarConsola(socketAceptado,mensaje,size);
		log_info(loggerConPantalla,"El programa ANSISOP enviado por socket: %d ha sido expulsado del sistema e se ha informado satifactoriamente",socketAceptado);
		break;
		case 'P':
			log_info(loggerConPantalla,"Iniciando rutina para imprimir por consola\n");
			recv(socketAceptado,&size,sizeof(int),0);
			mensaje=malloc(size);
			recv(socketAceptado,&mensaje,size,0);
			recv(socketAceptado,&pid,sizeof(int),0);

			socketHiloPrograma = buscarSocketHiloPrograma(pid);
			informarConsola(socketHiloPrograma,mensaje,size);
			log_info(loggerConPantalla,"Rutina para imprimir finalizo ----- PID: %d\n", pid);
			break;
		case 'M':
			log_info(loggerConPantalla,"Informando a Consola excepcion por grado de multiprogramacion\n");
			mensaje = "El programa ANSISOP no puede iniciar actualmente debido a grado de multiprogramacion, se mantendra en espera hasta poder iniciar";
			size=strlen(mensaje);
			informarConsola(socketAceptado,mensaje,size);
			break;
		case 'C':
			log_info(loggerConPantalla,"La CPU de socket %d se ha cerrado por  signal\n",socketAceptado);
			/*TODO: Eliminar la CPU que se desconecto. De la lista de CPUS y de la lista de SOCKETS*/
			break;
		case 'E':
			log_warning(loggerConPantalla,"\nLa Consola %d se ha cerrado",socketAceptado);
			pthread_mutex_lock(&mutexNuevoProceso);
			gestionarCierreConsola(socketAceptado);
			pthread_mutex_unlock(&mutexNuevoProceso);
			break;
		case 'F':
			log_info(loggerConPantalla,"La consola  %d  ha solicitado finalizar un proceso ",socketAceptado);
			recv(socketAceptado,&pid,sizeof(int),0);
			socketHiloPrograma = buscarSocketHiloPrograma(pid); /*TODO: Eliminar la Consola-PID de la lista de hilosProgramas*/
			buscarProcesoYTerminarlo(pid);
			mensaje = "Finalizar";
			size=strlen(mensaje);
			informarConsola(socketHiloPrograma,mensaje,size);
			log_info(loggerConPantalla,"Proceso finalizado-----PID: %d",pid);

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
		case 'O' :

			break;
		case 'D':
			log_info(loggerConPantalla,"Informando a Consola excepcion por planificacion detenido\n");
			mensaje = "El programa ANSISOP no puede iniciar actualmente debido a que la planificacion del sistema se encuentra detenido";
			size=strlen(mensaje);
			informarConsola(socketAceptado,mensaje,size);
			mensaje = "Finalizar";
			size=strlen(mensaje);
			informarConsola(socketAceptado,mensaje,size);
			break;
		default:
			break;
	}
}

void gestionarCierreConsola(int socket){
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
}

void cerrarHilosProgramas(char* procesosAFinalizar,int cantidad){
	log_info(loggerConPantalla,"Finalizando hilos programas de Consola");

	_Bool verificaPid(t_pcb* pcb){
		return pcb->pid==pid;
	}

	int i;
	int pid;

	for(i=0;i<cantidad;i++){
		pid = *((int*)procesosAFinalizar);

		pthread_mutex_lock(&mutexColaTerminados);
		procesosAFinalizar += sizeof(int);
		log_info(loggerConPantalla,"Finalizando hilo programa %d",pid);
		finalizarHiloPrograma(pid);
		pthread_mutex_unlock(&mutexColaTerminados);
	}

}



void eliminarSocket(int socket){
	pthread_mutex_lock(&mutex_FDSET);
	FD_CLR(socket,&master);
	pthread_mutex_unlock(&mutex_FDSET);
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

	pthread_mutex_lock(&mutexColaNuevos);
	if(list_any_satisfy(colaNuevos,(void*)verificarPid)){
		procesoATerminar=list_remove_by_condition(colaNuevos,(void*)verificarPid);
	}
	pthread_mutex_unlock(&mutexColaNuevos);


	pthread_mutex_lock(&mutexColaListos);

	if(list_any_satisfy(colaListos,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaListos,(void*)verificarPid);
			sem_wait(&sem_colaReady);
		}
	pthread_mutex_unlock(&mutexColaListos);


	cpuAFinalizar = list_find(listaCPU, (void*) verificarPidCPU);

	send(cpuAFinalizar->socket, &comandoFinalizar,sizeof(char),0);

	//printf()





















	/*pthread_mutex_lock(&mutexColaEjecucion); TODO: Implementar una vez de implementar la funcion EXPROPIAR()
	if(list_any_satisfy(colaEjecucion,(void*)verificarPid)){
		procesoAEliminar=list_remove_by_condition(colaEjecucion,(void*)verificarPid);
		t_cpu*cpu = list_remove_by_condition(listaCPU,(void*)verificarPidCPU);
		list_add(listaCPU,cpu);
		terminarProceso(cpu->socket);
		}
	pthread_mutex_unlock(&mutexColaEjecucion);*/

	pthread_mutex_lock(&mutexColaBloqueados);
	if(list_any_satisfy(colaBloqueados,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaBloqueados,(void*)verificarPid);
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

	log_info(loggerConPantalla, "Esperar: id: %s", semaforoAConsultar);
	int indice = indiceEnArray(semId, semaforoAConsultar);
	log_info(loggerConPantalla, "Indice encontrado: %d", indice);
	log_info(loggerConPantalla, "Valor en el indice: %d", semaforosGlobales[indice]);

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

	//FD_SET(0, &master); // Agrega el fd del teclado.

	FD_SET(socketServidor, &master); // add the listener to the master set
	maximoFD = socketServidor; // keep track of the biggest file descriptor so far, it's this one

	while(1) {
					pthread_mutex_lock(&mutex_FDSET);
					readFds = master;
					pthread_mutex_unlock(&mutex_FDSET);

					if (select(maximoFD + 1, &readFds, NULL, NULL, NULL) == -1) {
						perror("select");
						log_error(loggerSinPantalla,"Error en select\n");
						exit(2);
					}

					for (socket = 0; socket <= maximoFD; socket++) {
							if (FD_ISSET(socket, &readFds)) { //Hubo una conexion
									/*if(socket == 0){
										sem_post(&sem_ordenSelect);/*TODO: Cambiar esto a un mutex*/
										//break;
									//}
									if (socket == socketServidor) {
										addrlen = sizeof remoteaddr;
										nuevoFD = accept(socketServidor, (struct sockaddr *) &remoteaddr,&addrlen);

										if (nuevoFD == -1) perror("accept");

										else {

											FD_SET(nuevoFD, &master);

											if (nuevoFD > maximoFD)	maximoFD = nuevoFD;

											log_info(loggerConPantalla,"\nSelectserver: nueva conexion en IP: %s en socket %d\n",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), nuevoFD);
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

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}
