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
#include "interfazHandler.h"
#include "sincronizacion.h"
#include "planificacion.h"
#include "configuraciones.h"
#include "conexionMemoria.h"
#include "contabilidad.h"
#include "semaforosAnsisop.h"
#include "comandosCPU.h"
#include "heap.h"
#include "sockets.h"
#include "listasAdministrativas.h"

#include "capaFilesystem.h"
#include "excepciones.h"


typedef struct{
	pthread_t hilo;
	int pid;
}t_hilo;

t_list* listaHilos;

void signalHandler(int signum);
void cerrarTodo();
//--------ConnectionHandler--------//
void connectionHandler(int socket, char orden);
void inicializarListas();
int atenderNuevoPrograma(int socketAceptado);
t_codigoPrograma* recibirCodigoPrograma(int socketHiloConsola);
void gestionarNuevaCPU(int socketCPU,int quantum);
void gestionarRRFinQuantum(int socket);
void handShakeCPU(int socketCPU);
//---------ConnectionHandler-------//

//------InterruptHandler-----//
void interruptHandler(int socket,char orden);
void imprimirPorConsola(int socketAceptado);
void gestionarFinalizarProgramaConsola(int socket);
int buscarProcesoYTerminarlo(int pid);
void gestionarCierreConsola(int socket);
void gestionarCierreCpu(int socketCpu);
void gestionarAlocar(int socket);
void gestionarLiberar(int socket);
//------InterruptHandler-----//


//---------Conexiones---------------//
void selectorConexiones();
int flagFinalizarKernel = 0;
//---------Conexiones-------------//


//----------shared vars-----------//
void obtenerValorDeSharedVar(int socket);
int* variablesGlobales;
pthread_mutex_t mutexVariablesGlobales = PTHREAD_MUTEX_INITIALIZER;
int indiceEnArray(char** array, char* elemento);
void obtenerVariablesCompartidasDeLaConfig();
void guardarValorDeSharedVar(int socket);
//----------shared vars-----------//



int main() {

	flagPlanificacion = 1;
	flagHuboAlgunProceso = 0;
	flagCPUSeDesconecto = 0;

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	imprimirConfiguraciones();
	imprimirInterfazUsuario();
	inicializarSockets();
	inicializarSemaforos();

	inicializarLog("/home/utnso/Log/logKernel.txt");
	inicializarListas();
	inicializarExitCodeArray();
	handshakeMemoria();
	obtenerVariablesCompartidasDeLaConfig();
	obtenerSemaforosANSISOPDeLasConfigs();

	pthread_create(&planificadorCortoPlazo, NULL,(void*)planificarCortoPlazo,NULL);
	pthread_create(&planificadorMedianoPlazo, NULL,(void*)planificarMedianoPlazo,NULL);
	pthread_create(&planificadorLargoPlazo, NULL,(void*)planificarLargoPlazo,NULL);
	pthread_create(&interfaz, NULL,(void*)interfazHandler,NULL);

	signal(SIGINT,signalHandler);


	selectorConexiones();
	return 0;
}

void signalHandler(int signum){
	if(signum== SIGINT){
		log_error(loggerConPantalla,"El proceso Kernel se ha abortado");
		cerrarTodo();
	}
}

void cerrarTodo(){
	log_error(loggerConPantalla,"Iniciando rutina de cierre");
	char comandoSalir = 'X';
	send(socketFyleSys,&comandoSalir,sizeof(char),0);
	/*Limpiar los pcbs*/
	/*Recibir todos los pcb en ejecucion*/
	/*Avisar a Memoria que se desconecta*/
	/*Hacer signal a todos los hilos, y setear los flags para que terminen*/
	exit(1);
}

void connectionHandler(int socket, char orden) {
	int valor;
	int quantum = 0; //FIFO--->0 ; RR != 0
	switch (orden) {
		case 'A':	atenderNuevoPrograma(socket);
					break;
		case 'N':	gestionarNuevaCPU(socket,quantum);
					break;
		case 'T':	gestionarFinalizacionProgramaEnCpu(socket);
					break;
		case 'F':	/*TODO: Crear un hilo para cada servicio de FS*/
					//printf("Yendo a FS\n");
					interfaceHandlerFileSystem(socket);//En vez de la V , poner el recv de la orden que quieras hacer con FS
					break;
		case 'P':	handShakeCPU(socket);
					break;
		case 'X':
					recv(socket,&orden,sizeof(char),0);
					interruptHandler(socket,orden);
					break;
		case 'R':	gestionarRRFinQuantum(socket);
					break;
		case 'Z':	eliminarSocket(socket);
					break;
		default:
					log_error(loggerConPantalla,"Orden %c no definida", orden);
					break;
		}
	return;
}

int atenderNuevoPrograma(int socketAceptado){
		log_info(loggerConPantalla,"Atendiendo nuevo programa");

		contadorPid++;
		send(socketAceptado,&contadorPid,sizeof(int),0);

		t_codigoPrograma* codigoPrograma = recibirCodigoPrograma(socketAceptado);
		t_pcb* proceso=crearPcb(codigoPrograma->codigo,codigoPrograma->size);
		codigoPrograma->pid=proceso->pid;
		log_info(loggerConPantalla,"Pcb encolado en Nuevos--->PID: %d",proceso->pid);

		if(!flagPlanificacion) {
					contadorPid--;
					free(proceso);
					log_warning(loggerConPantalla,"La planificacion del sistema esta detenida");
					interruptHandler(codigoPrograma->socketHiloConsola,'B'); // Informa a consola error por planificacion detenida
					free(codigoPrograma);
					return -1;
						}
		cargarConsola(proceso->pid,codigoPrograma->socketHiloConsola);
		crearInformacionContable(proceso->pid);

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
	codigoPrograma->codigo = malloc(codigoPrograma->size + sizeof(char));
	recv(socketHiloConsola,codigoPrograma->codigo,codigoPrograma->size,0);
	strcpy(codigoPrograma->codigo + codigoPrograma->size , "\0");
	codigoPrograma->socketHiloConsola=socketHiloConsola;
	return codigoPrograma;
}

void handShakeCPU(int socketCPU){
	send(socketCPU,&config_paginaSize,sizeof(int),0);
	send(socketCPU,&stackSize,sizeof(int),0);
}



void gestionarNuevaCPU(int socketCPU,int quantum){

	if(!strcmp(config_algoritmo, "RR")) quantum = config_quantum;
	send(socketCPU,&quantum,sizeof(int),0);

	t_cpu* cpu = malloc(sizeof(t_cpu));
	cpu->socket = socketCPU;
	cpu->estado = OCIOSA;
	//cpu->socketInterrupciones = socketCPU +1;


	pthread_mutex_lock(&mutexListaCPU);
	list_add(listaCPU,cpu);
	pthread_mutex_unlock(&mutexListaCPU);
	sem_post(&sem_CPU);
}

void gestionarRRFinQuantum(int socket){
	t_pcb* pcb;
	t_cpu *cpu;
	int cantidadDeRafagas;
	int cpuFinalizada;

	_Bool verificaSocket(t_cpu* unaCpu){
		return (unaCpu->socket == socket);
	}


	cpu = list_find(listaCPU, (void*)verificaSocket);


	if(cpu->estado != FQPB){

		recv(socket,&cpuFinalizada, sizeof(int),0);
		recv(socket,&cantidadDeRafagas,sizeof(int),0);

		pthread_mutex_lock(&mutexRecibirPCB);
		pcb = recibirYDeserializarPcb(socket);
		pthread_mutex_unlock(&mutexRecibirPCB);

		cambiarEstadoCpu(socket,OCIOSA);
		actualizarRafagas(pcb->pid,cantidadDeRafagas);
		removerDeColaEjecucion(pcb->pid);
		agregarAFinQuantum(pcb);
		if(cpuFinalizada != 0) sem_post(&sem_CPU);
	}

}

void interruptHandler(int socketAceptado,char orden){
	log_warning(loggerConPantalla,"Ejecutando interrupt handler");

	switch(orden){

		case 'B':	excepcionPlanificacionDetenida(socketAceptado);
					break;
		case 'C':	gestionarCierreCpu(socketAceptado);
					break;
		case 'E':	gestionarCierreConsola(socketAceptado);
					break;
		case 'F':	gestionarFinalizarProgramaConsola(socketAceptado);
					break;
		case 'P':	imprimirPorConsola(socketAceptado);
					break;
		case 'R':	gestionarAlocar(socketAceptado);
					break;
		case 'L':	gestionarLiberar(socketAceptado);
					break;
		case 'K':	excepcionStackOverflow(socketAceptado);
					break;
		case 'M':
					excepcionDireccionInvalida(socketAceptado);
					break;
		case 'O':	obtenerValorDeSharedVar(socketAceptado);
					break;
		case 'G':	guardarValorDeSharedVar(socketAceptado);
					break;
		case 'W':	waitSemaforoAnsisop(socketAceptado);
					break;
		case 'S':	signalSemaforoAnsisop(socketAceptado);
					break;
		default:
			break;
	}
	log_warning(loggerConPantalla,"Interrupt Handler finalizado");
}


void gestionarCierreCpu(int socketCpu){
	log_warning(loggerConPantalla,"Gestionando cierre de CPU:%d",socketCpu);
	_Bool verificaSocket(t_cpu* cpu){
		return cpu->socket == socketCpu;
	}
	int socketInterrupciones;
	t_cpu* cpu;
	/*TODO: Saque el WAIT porque en el peor de los casos, el planificador se activara, vera que no hay cpus ociosas, y guardara devuelta el pcb en la cola de listos*/



	pthread_mutex_lock(&mutexListaCPU);
	cpu = list_remove_by_condition(listaCPU,(void*)verificaSocket);
	pthread_mutex_unlock(&mutexListaCPU);
//	socketInterrupciones = cpu->socketInterrupciones;
	eliminarSocket(socketCpu);
	//eliminarSocket(socketInterrupciones);
	free(cpu);
	log_error(loggerConPantalla,"La CPU %d se ha cerrado",socketCpu);
}

void imprimirPorConsola(socketAceptado){
	char* mensaje;
	int size;
	recv(socketAceptado,&size,sizeof(int),0);
	mensaje=malloc(size);
	recv(socketAceptado,mensaje,size,0);
	recv(socketAceptado,&pid,sizeof(int),0);
	log_info(loggerConPantalla,"Imprimiendo por consola--->PID:%d--->Mensaje: %s",pid,mensaje);
	informarConsola(buscarSocketHiloPrograma(pid),mensaje,size);
	free(mensaje);
}

void gestionarCierreConsola(int socket){
	log_warning(loggerConPantalla,"Gestionando cierre de consola %d",socket);
	pthread_mutex_lock(&mutexNuevoProceso);
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
						buscarProcesoYTerminarlo(pid);
						desplazamiento ++;
			}

			send(socket,&i,sizeof(int),0); // a modo de ok
			log_warning(loggerConPantalla,"Consola %d cerrada",socket);
			eliminarSocket(socket);
			free(procesosAFinalizar);
			pthread_mutex_unlock(&mutexNuevoProceso);
}

void gestionarFinalizarProgramaConsola(int socket){
	log_warning(loggerConPantalla,"La consola  %d  ha solicitado finalizar un proceso ",socket);
	recv(socket,&pid,sizeof(int),0);
	finalizarProcesoVoluntariamente(pid);
}

int buscarProcesoYTerminarlo(int pid){
	log_info(loggerConPantalla,"Finalizando proceso--->PID: %d ",pid);

	t_pcb *procesoATerminar;
	t_cpu *cpuAFinalizar = malloc(sizeof(t_cpu));

	_Bool verificarPid(t_pcb* pcb){
			return (pcb->pid==pid);
		}
	_Bool verificarPidCPU(t_cpu* cpu){
				return (cpu->pid==pid);
			}

	_Bool verificarPidHilo(t_hilo* hilo){
					return (hilo->pid==pid);
				}

	pthread_mutex_lock(&mutexColaEjecucion);
		if(list_any_satisfy(colaEjecucion,(void*)verificarPid)){
			pthread_mutex_unlock(&mutexColaEjecucion);

			pthread_mutex_lock(&mutexListaCPU);
			if(list_any_satisfy(listaCPU,(void*)verificarPidCPU)) cpuAFinalizar = list_find(listaCPU, (void*) verificarPidCPU);
			pthread_mutex_unlock(&mutexListaCPU);

			pthread_mutex_lock(&mutexListaHilos);
			if(list_any_satisfy(listaHilos,(void*)verificarPidHilo)){
					t_hilo* hilo=list_remove_by_condition(listaHilos,(void*)verificarPidHilo);

					pthread_mutex_lock(&mutexMemoria); /*TODO: Para garantizarme que no se este ejecutando un servicio a Memoria*/
					pthread_kill(hilo->hilo,SIGUSR1); // Seria lo mismo con FS
					pthread_mutex_unlock(&mutexMemoria);
			}
			pthread_mutex_unlock(&mutexListaHilos);

			procesoATerminar = expropiarVoluntariamente(cpuAFinalizar->socket);
		}

	pthread_mutex_lock(&mutexColaNuevos);
	if(list_any_satisfy(colaNuevos,(void*)verificarPid)){
		procesoATerminar=list_remove_by_condition(colaNuevos,(void*)verificarPid);
	}
	pthread_mutex_unlock(&mutexColaNuevos);


	pthread_mutex_lock(&mutexColaListos);

	if(list_any_satisfy(colaListos,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaListos,(void*)verificarPid);
			liberarRecursosEnMemoria(procesoATerminar);
			sem_wait(&sem_colaListos);
		}
	pthread_mutex_unlock(&mutexColaListos);

	pthread_mutex_lock(&mutexColaBloqueados);
	if(list_any_satisfy(colaBloqueados,(void*)verificarPid)){
			procesoATerminar=list_remove_by_condition(colaBloqueados,(void*)verificarPid);
			liberarRecursosEnMemoria(procesoATerminar); /*TODO: Tambien habria que limpiarlo de la cola del semaforo asociado*/
		}
	pthread_mutex_unlock(&mutexColaBloqueados);

	procesoATerminar->exitCode = exitCodeArray[EXIT_END_OF_PROCESS]->value;
	terminarProceso(procesoATerminar);

	return 0;
}

void gestionarAlocar(int socket){
	int size,pid;
    pthread_t heapThread;
	recv(socket,&pid,sizeof(int),0);
	log_info(loggerConPantalla,"Gestionando reserva de memoria dinamica--->PID:%d",pid);
	actualizarSysCalls(pid);
	recv(socket,&size,sizeof(int),0);

	if(size > config_paginaSize - sizeof(t_bloqueMetadata)*2) {
		excepcionPageSizeLimit(socket,pid);
		return;
	}

	t_alocar* data= malloc(sizeof(t_alocar));
	data->pid = pid;
	data->size = size;
	data->socket = socket;
	int err=pthread_create(&heapThread,NULL,(void*) reservarEspacioHeap,data);
	if(err){
		printf("ERROR; return code from pthread_create() is %d\n", err);
		return;
	}

	t_hilo* hilo = malloc(sizeof(t_hilo));
	hilo->pid = pid;
	hilo->hilo = heapThread;
	pthread_mutex_lock(&mutexListaHilos);
	list_add(listaHilos,hilo);
	pthread_mutex_unlock(&mutexListaHilos);

	actualizarAlocar(pid,size);
}

void gestionarLiberar(int socket){
	int pid,pagina,offset;
	recv(socket,&pid,sizeof(int),0);
	log_info(loggerConPantalla,"Gestionando liberacion de memoria dinamica--->PID:%d",pid);
	recv(socket,&pagina,sizeof(int),0);
	recv(socket,&offset,sizeof(int),0);

	liberarBloqueHeap(pid,pagina,offset);
	actualizarSysCalls(pid);
	int resultadoEjecucion = 1;
	send(socket,&resultadoEjecucion,sizeof(int),0);
}

void enviarAImprimirALaConsola(int socketConsola, void* buffer, int size){
	void* mensajeAConsola = malloc(sizeof(int)*2 + sizeof(char));

	memcpy(mensajeAConsola,&size, sizeof(char));
	memcpy(mensajeAConsola + sizeof(int)+ sizeof(char), buffer,size);
	send(socketConsola,mensajeAConsola,sizeof(char)+ sizeof(int),0);
}



void inicializarListas(){
	colaNuevos = list_create();
	colaListos = list_create();
	colaTerminados = list_create();
	colaEjecucion = list_create();
	colaBloqueados = list_create();

	listaAdmHeap = list_create();

	listaConsolas = list_create();
	listaCPU = list_create();

	listaCodigosProgramas = list_create();
	//listaTablasArchivosPorProceso = list_create();

	listaFinQuantum = list_create();
	listaEspera = list_create();
	listaContable = list_create();

	listaSemaforosGlobales = list_create();
	listaSemYPCB = list_create();

	listaHilos = list_create();

	tablaArchivosGlobal = list_create();
	listaTablasProcesos = list_create();
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
	int pid;
	recv(socket,&pid,sizeof(int),0);
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

	actualizarSysCalls(pid);
}

void guardarValorDeSharedVar(int socket){
	int tamanio, valorAGuardar;
	int pid;
	char* identificador;
	recv(socket,&pid,sizeof(int),0);
	recv(socket,&tamanio,sizeof(int),0);
	identificador = malloc(tamanio);
	recv(socket,identificador,tamanio,0);
	recv(socket,&valorAGuardar,sizeof(int),0);
	log_info(loggerConPantalla, "Guardar Valor de : id: %s, valor: %d", identificador, valorAGuardar);
	int indice = indiceEnArray(shared_vars, identificador);
	pthread_mutex_lock(&mutexVariablesGlobales);
	variablesGlobales[indice] = valorAGuardar;
	pthread_mutex_unlock(&mutexVariablesGlobales);
	actualizarSysCalls(pid);
}

int indiceEnArray(char** array, char* elemento){
	int i = 0;
	while(array[i] && strcmp(array[i], elemento)) i++;

	return array[i] ? i:-1;
}

void selectorConexiones() {
	log_info(loggerConPantalla,"Iniciando selector de conexiones");
	int nuevoFD;
	int socket;
	char orden;
	int maximoFD;
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

										pthread_mutex_lock(&mutex_masterSet);
										FD_SET(nuevoFD, &master);
										pthread_mutex_unlock(&mutex_masterSet);

										if (nuevoFD > maximoFD)	maximoFD = nuevoFD;
										log_info(loggerConPantalla,"Nueva conexion en IP: %s en socket %d",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), nuevoFD);
									}
									else {
											recv(socket, &orden, sizeof(char), 0);

											connectionHandler(socket, orden);
									}
							}
					}
		}
	log_info(loggerConPantalla,"Finalizando selector de conexiones");
}

