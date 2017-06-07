/*
 * interfazHandler.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */
#include "sincronizacion.h"
#include "planificacion.h"
#include "configuraciones.h"
#include "capaFS.h"

void interfazHandler();
void imprimirInterfazUsuario();
void modificarGradoMultiprogramacion();
void obtenerListadoProcesos();
void mostrarProcesos(char orden);
void imprimirListadoDeProcesos(t_list* listaPid);
void filtrarPorPidYMostrar(t_list* cola);
void interfazHandlerParaFileSystem(char orden,int socket_aceptado);


pthread_t interfaz;

/*-------------LOG-----------------*/
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;


void interfazHandler(){
	char orden;
	char *mensajeRecibido;

	while(1){
		sem_wait(&sem_ordenSelect);
		read(0,&orden,sizeof(char));

		switch(orden){
				case 'O':
					obtenerListadoProcesos();
					break;
				case 'P':
					if(orden == 'P' && flagPlanificacion == 0){
						log_warning(loggerConPantalla, "Planificacion ya pausada");
						break;
					}
					log_info(loggerConPantalla, "Se pauso la planificacion");
					pausarPlanificacion();
					orden = '\0';
					break;
				case 'R':
					if(orden == 'R' && flagPlanificacion == 1){
						log_warning(loggerConPantalla, "Planificacion no se encuentra pausada");
						break;
					}
					log_info(loggerConPantalla, "Se reanudo la planificacion");
					reanudarPLanificacion();
					orden = '\0';
					break;
				case 'G':
					/*mostrarTablaGlobalArch(); TODO HAY QUE IMPLEMENTAR*/
					break;
				case 'M':
					modificarGradoMultiprogramacion();
					break;
				case 'K':
					/*finalizarProceso(int pid) TODO HAY QUE IMPLEMENTAR*/
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
				case 'I':
						imprimirInterfazUsuario();
						break;
				case 'F':
					printf("Enviando instrucciones a File System");
					interfazHandlerParaFileSystem('V',1);
					break;
				case 'Z':
					kill(pid,SIGUSR2);
					break;
				default:
					log_warning(loggerConPantalla ,"\nOrden no reconocida\n");
					break;
		}
	}

}




void obtenerListadoProcesos(){
	char orden;
	read(0,&orden,sizeof(char));
	switch(orden){
	case 'T':
		orden = 'T';
		mostrarProcesos(orden);
		break;
	case 'C':
		read(0,&orden,sizeof(char));
		mostrarProcesos(orden);
		break;
	default:
		log_error(loggerConPantalla,"Orden no reconocida\n");
		break;
	}
}

void mostrarProcesos(char orden){

	int transformarPid(t_pcb* pcb){
				return pcb->pid;
			}

	switch(orden){
	case 'N':
		log_info(loggerConPantalla,"Procesos en estado ---> NEW");
		pthread_mutex_lock(&mutexColaNuevos);
		imprimirListadoDeProcesos(list_map(colaNuevos,(void*)transformarPid));
		pthread_mutex_unlock(&mutexColaNuevos);
		break;
	case 'R':
		log_info(loggerConPantalla,"Procesos en estado ---> READY");
		pthread_mutex_lock(&mutexColaListos);
		imprimirListadoDeProcesos(list_map(colaListos,(void*)transformarPid));
		pthread_mutex_unlock(&mutexColaListos);
		break;
	case 'E':
		log_info(loggerConPantalla,"Procesos en estado ---> EXEC");
		pthread_mutex_lock(&mutexColaEjecucion);
		imprimirListadoDeProcesos(list_map(colaEjecucion,(void*)transformarPid));
		pthread_mutex_unlock(&mutexColaEjecucion);
		break;
	case 'F':
		log_info(loggerConPantalla,"Procesos en estado ---> FINISHED");
		pthread_mutex_lock(&mutexColaTerminados);
		imprimirListadoDeProcesos(list_map(colaTerminados,(void*)transformarPid));
		pthread_mutex_unlock(&mutexColaTerminados);
		break;
	case 'B':
		log_info(loggerConPantalla,"Procesos en estado ---> BLOCKED");
		pthread_mutex_lock(&mutexColaBloqueados);
		imprimirListadoDeProcesos(list_map(colaBloqueados,(void*)transformarPid));
		pthread_mutex_unlock(&mutexColaBloqueados);
		break;
	case 'T':
		break;
	default:
		break;
	}
}

void imprimirListadoDeProcesos(t_list* listaPid){
	printf("Cantidad de procesos: %d\n", listaPid->elements_count);
	printf("\tPID\n");
	int pid;
	int i;
	for(i=0 ; i<listaPid->elements_count ; i++){
		pid = list_get(listaPid,i);
		printf("\t%d\n",pid);
	}
	list_destroy(listaPid);
}

void modificarGradoMultiprogramacion(){
	int nuevoGrado;
	log_info(loggerConPantalla,"Ingresar nuevo grado de multiprogramacion\n");
	scanf("%d",&nuevoGrado);
	pthread_mutex_lock(&mutexGradoMultiProgramacion);
	config_gradoMultiProgramacion= nuevoGrado;
	pthread_mutex_unlock(&mutexGradoMultiProgramacion);
	log_info(loggerConPantalla,"Se cambio el GradoMultiProg a:%d\n",nuevoGrado);
}

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}

void imprimirInterfazUsuario(){

	/**************************************Printea interfaz Usuario Kernel*******************************************************/
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	printf("Para realizar acciones permitidas en la consola Kernel, seleccionar una de las siguientes opciones\n");
	printf("\nIngresar orden de accion:\nO - Obtener listado programas\n\tT - Obtener todos los procesos\n\tC - Obtener procesos de un estado\n\t\tN - New\n\t\tR - Ready\n\t\tE - Exec\n\t\tB - Blocked\n\t\tF - Finished\nP - Pausar planificacion\nR - Reanudar planificacion\nG - Mostrar tabla global de archivos\nM - Modif grado multiprogramacion\nK - Finalizar proceso\n");
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	/****************************************************************************************************************************/
}
