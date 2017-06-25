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
void finalizarProcesoVoluntariamente(int pid);
void obtenerListadoProcesos();
void obtenerDatosProceso(int pid);
void mostrarProcesos(char orden);
void imprimirListadoDeProcesos(t_list* listaPid);
void filtrarPorPidYMostrar(t_list* cola);
void interfazHandlerParaFileSystem(char orden,int socket_aceptado);
int verificarProcesoExistente(int pid);

pthread_t interfaz;
int flagTerminarUI=0;
/*-------------LOG-----------------*/
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;


void interfazHandler(){
	char orden;
	char *mensajeRecibido;
	int pid;


	while(1){
		scanf("%c",&orden);
		switch(orden){

				case 'L':
					obtenerListadoProcesos();
					break;
				case 'O':
					printf("Ingrese el pid del proceso\n");
					scanf("%d",&pid);
					if(verificarProcesoExistente(pid)<0){
						log_error(loggerConPantalla,"Proceso no existente");
						break;
					}
					printf("Datos del proceso:%d\t\t\n",pid);
					printf("PID\tCantidad de Rafagas\tCantidad de SysCalls\tPaginas de Heap\t\tCantidad Alocar\tSize Alocar\tCantidad Liberar\tSize Liberar\n");
					obtenerDatosProceso(pid);
					break;
				case 'R':
					reanudarPLanificacion();
					printf("FLAG PLANIFICACION: %d\n", flagPlanificacion);
					break;
				case 'P':
					pausarPlanificacion();
					printf("FLAG PLANIFICACION: %d\n", flagPlanificacion);
					break;
				case 'G':
					/*mostrarTablaGlobalArch(); TODO HAY QUE IMPLEMENTAR*/
					break;
				case 'M':
					pthread_mutex_lock(&mutexNuevoProceso);
					modificarGradoMultiprogramacion();
					pthread_mutex_unlock(&mutexNuevoProceso);

					break;
				case 'K':
					printf("Ingrese el pid del proceso a finalizar\n");
					scanf("%d",&pid);
					/*TODO: Corroborar que no haya sido eliminado antes*/

					finalizarProcesoVoluntariamente(pid);
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
					interfazHandlerParaFileSystem('A',1);
					break;
				default:
					log_error(loggerConPantalla ,"Orden no reconocida");
					break;
		}
	}

}

int verificarProcesoExistente(int pid){
	int resultado;
	_Bool verificaPid(t_contable* proceso){
			return proceso->pid == pid;
		}
	pthread_mutex_lock(&mutexListaContable);
	if(list_any_satisfy(listaContable,(void*)verificaPid)) resultado=0;
	else resultado=-1;
	pthread_mutex_unlock(&mutexListaContable);
	return resultado;
}

void obtenerDatosProceso(int pid){

	_Bool verificaPid(t_contable* proceso){
		return proceso->pid == pid;
	}
	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPid);

	printf("%d\t\t%d\t\t\t%d\t\t\t%d\t\t\t%d\t\t%d\t\t%d\t\t\t%d\n",pid,proceso->cantRafagas,proceso->cantSysCalls,proceso->cantPaginasHeap,proceso->cantAlocar,
			proceso->sizeAlocar,proceso->cantLiberar,proceso->sizeLiberar);

	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);
}


void obtenerListadoProcesos(){
	char orden;
	scanf("%c",&orden);
	switch(orden){
	case 'T':
		orden = 'T';
		mostrarProcesos(orden);
		break;
	case 'C':
		scanf("%c",&orden);
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
	printf("\tPID\tCantidad de Rafagas\tCantidad de SysCalls\tPaginas de Heap\n");
	int pid;
	int i;
	for(i=0 ; i<listaPid->elements_count ; i++){
		pid = list_get(listaPid,i);
		obtenerDatosProceso(pid);
	}
	list_destroy(listaPid);
}



void finalizarProcesoVoluntariamente(int pid){
	pthread_mutex_lock(&mutexNuevoProceso);
	if(buscarProcesoYTerminarlo(pid)<0){
		pthread_mutex_unlock(&mutexNuevoProceso);
		return;
	}
	finalizarHiloPrograma(pid);
	pthread_mutex_unlock(&mutexNuevoProceso);
	log_info(loggerConPantalla,"Proceso finalizado-----PID: %d",pid);
}


void modificarGradoMultiprogramacion(){ /*TODO: Ver de dejar cambiar a uno menor*/
	int nuevoGrado;

	log_info(loggerConPantalla,"Ingresar nuevo grado de multiprogramacion\n");
	scanf("%d",&nuevoGrado);
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	if(nuevoGrado < gradoMultiProgramacion) {
		log_error(loggerConPantalla,"El valor ingresado es menor a la cantidad de procesos en el sistema actualmente");
		pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
		return;
	}
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);

	pthread_mutex_lock(&mutex_config_gradoMultiProgramacion);
	config_gradoMultiProgramacion= nuevoGrado;
	pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);

	sem_post(&sem_admitirNuevoProceso);
	log_info(loggerConPantalla,"Se cambio la configuracion del Grado de Multiprogramacion a:%d\n",nuevoGrado);
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
	printf("\nIngresar orden de accion:\nO - Obtener datos de proceso\nL - Obtener listado programas\n\tT - Obtener todos los procesos\n\tC - Obtener procesos de un estado\n\t\tN - New\n\t\tR - Ready\n\t\tE - Exec\n\t\tB - Blocked\n\t\tF - Finished\nP - Pausar planificacion\nR - Reanudar planificacion\nG - Mostrar tabla global de archivos\nM - Modif grado multiprogramacion\nK - Finalizar proceso\n");
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	/****************************************************************************************************************************/
}
