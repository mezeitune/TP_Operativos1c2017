/*
 * interfazHandler.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */
#include "sincronizacion.h"
#include "planificacion.h"
#include "configuraciones.h"
#include "capaFilesystem.h"

void interfazHandler();
void imprimirInterfazUsuario();

void interfaceObtenerListadoProcesos();
void interfaceObtenerDatosProceso();
void interfaceFinalizarProcesoVoluntariamente();

void interfaceTablaGlobalArchivos();

void interfaceSolicitarContenidoMemoria();

void interfaceModificarGradoMultiprogramacion();

void finalizarProcesoVoluntariamente(int pid);
void imprimirListadoDeProcesos(t_list* procesos);
void filtrarPorPidYMostrar(t_list* cola);
void interfazHandlerParaFileSystem(char orden,int socket_aceptado);
int verificarProcesoExistente(int pid);
int verificarProcesoNoTerminado(int pid);


void obtenerDatosProceso(int pid);
void mostrarProcesos(char orden);
void mostrarTodosLosProcesos();

void imprimirDatosContables(t_contable* proceso);
void imprimirTablaArchivosProceso(int pid);

pthread_t interfaz;
int flagTerminarUI=0;
/*-------------LOG-----------------*/
void inicializarLog(char *rutaDeLog);
t_log *logKernel;
t_log *logKernel;


void interfazHandler(){
	char orden;
	int cont = 0;
	while(1){
		scanf("%c",&orden);
		cont++;
		switch(orden){
				case 'L': 	interfaceObtenerListadoProcesos();
							break;
				case 'O': 	interfaceObtenerDatosProceso();
							break;
				case 'R':	interfazReanudarPlanificacion();
							break;
				case 'P':	interfazPausarPlanificacion();
							break;
				case 'G':	interfaceTablaGlobalArchivos();
							break;
				case 'M':	interfaceModificarGradoMultiprogramacion();
							break;
				case 'K':	interfaceFinalizarProcesoVoluntariamente();
							break;
				case 'S':	interfaceSolicitarContenidoMemoria();
							break;
				case 'I':	imprimirInterfazUsuario();
							break;
				case 'F':
						testEscribirArchivo();
					//interfazHandlerParaFileSystem('A',1);
					break;
				case 'Z':
						testLeerArchivo();
					break;
				case 'X':
						testBorrarArchivo();
					break;
				default:
					if(cont!=2)log_error(logKernel ,"Orden no reconocida");
					else cont = 0;
					break;
		}
	}

}


void interfaceObtenerDatosProceso(){
	printf("Ingrese el pid del proceso\n");
	scanf("%d",&pid);
		if(verificarProcesoExistente(pid)<0){
			log_error(logKernel,"Proceso no existente---> PID: %d", pid);
			return;
		}
	printf("Datos del proceso--->%d\n",pid);
	printf("PID\tCantidad de Rafagas\tCantidad de SysCalls\tPaginas de Heap\t\tCantidad Alocar\tSize Alocar\tCantidad Liberar\tSize Liberar\n");
	obtenerDatosProceso(pid);
}

void interfaceFinalizarProcesoVoluntariamente(){
	printf("Ingrese el pid del proceso a finalizar\n");
	scanf("%d",&pid);
			if(verificarProcesoExistente(pid)<0){
				log_error(logKernel,"Proceso no existente---> PID: %d", pid);
				return;
				}
			if(verificarProcesoNoTerminado(pid)<0){
				log_error(logKernel,"Proceso ya finalizado--->PID: %d", pid);
				return;
				}
	finalizarProcesoVoluntariamente(pid);
}


void interfaceSolicitarContenidoMemoria(){
	char* mensaje;
	if((solicitarContenidoAMemoria(&mensaje))<0){
			printf("No se pudo solicitar el contenido\n");
			return;
		}
	printf("El mensaje recibido de la Memoria es : %s\n" , mensaje);
}


int verificarProcesoNoTerminado(int pid){
	int resultado;
	_Bool verificaPid(t_pcb* proceso){
		return proceso->pid == pid;
	}

	pthread_mutex_lock(&mutexColaTerminados);
	if(list_any_satisfy(colaTerminados,(void*)verificaPid)) resultado = -1;
	else resultado = 0;
	pthread_mutex_unlock(&mutexColaTerminados);
	return resultado;
}

int verificarProcesoExistente(int pid){
	int existe;
	_Bool verificaPid(t_pcb* proceso){
			return proceso->pid == pid;
		}
	pthread_mutex_lock(&mutexColaNuevos);
	if(list_any_satisfy(colaNuevos,(void*)verificaPid)) existe= 1;
	pthread_mutex_unlock(&mutexColaNuevos);

	pthread_mutex_lock(&mutexColaListos);
	if(list_any_satisfy(colaListos,(void*)verificaPid)) existe= 1;
	pthread_mutex_unlock(&mutexColaListos);

	pthread_mutex_lock(&mutexColaEjecucion);
	if(list_any_satisfy(colaEjecucion,(void*)verificaPid)) existe= 1;
	pthread_mutex_unlock(&mutexColaEjecucion);

	pthread_mutex_lock(&mutexColaBloqueados);
	if(list_any_satisfy(colaBloqueados,(void*)verificaPid)) existe= 1;
	pthread_mutex_unlock(&mutexColaBloqueados);

	pthread_mutex_lock(&mutexColaTerminados);
	if(list_any_satisfy(colaTerminados,(void*)verificaPid)) existe= 1;
	pthread_mutex_unlock(&mutexColaTerminados);


	return existe;
}

void obtenerDatosProceso(int pid){ /*TODO: Mutex tablas*/
	_Bool verificaPid(t_contable* proceso){
		return proceso->pid == pid;
	}

	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPid);

	imprimirDatosContables(proceso);

	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);
	imprimirTablaArchivosProceso(pid);
}

void imprimirDatosContables(t_contable* proceso){
	log_info(logKernel,"%d\t\t%d\t\t\t%d\t\t\t%d\t\t\t%d\t\t%d\t\t%d\t\t\t%d\n",pid,proceso->cantRafagas,proceso->cantSysCalls,proceso->cantPaginasHeap,proceso->cantAlocar,
				proceso->sizeAlocar,proceso->cantLiberar,proceso->sizeLiberar);
}

void imprimirTablaArchivosProceso(int pid){

	_Bool verificaPidArchivo(t_indiceTablaProceso* entrada){
		return entrada->pid == pid;
	}
	int i;
	log_info(logKernel,"\t\t\tTabla de archivos del proceso\n");
	log_info(logKernel,"\t\t\tFile Descriptor\tFlags\tIndice Global\tCursor\n");

	t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPidArchivo);
	t_entradaTablaProceso* entrada;

	for(i=0;i<entradaTablaProceso->tablaProceso->elements_count;i++){
		entrada = list_get(entradaTablaProceso->tablaProceso,i);
		log_info(logKernel,"\t\t\t\t%d\t%s\t\t%d\t%d\n",entrada->fd,entrada->flags,entrada->globalFd,entrada->puntero);
	}
	list_add(listaTablasProcesos,entradaTablaProceso);
}

void interfaceObtenerListadoProcesos(){
	char orden;
	scanf("%c",&orden);
	switch(orden){
	case 'T':
		mostrarTodosLosProcesos();
		break;
	case 'C':
		scanf("%c",&orden);
		mostrarProcesos(orden);
		break;
	default:
		log_error(logKernel,"Orden no reconocida-->ORDEN: %c\n", orden);
		break;
	}
}

void mostrarTodosLosProcesos(){
	mostrarProcesos('N');
	mostrarProcesos('R');
	mostrarProcesos('E');
	mostrarProcesos('B');
	mostrarProcesos('F');
}

void mostrarProcesos(char orden){ /*TODO: Cambiar logs a prints*/

	int transformarPid(t_pcb* pcb){
				return pcb->pid;
			}

	switch(orden){
	case 'N':
		printf("Procesos en estado ---> NEW\n");
		pthread_mutex_lock(&mutexColaNuevos);
		imprimirListadoDeProcesos(colaNuevos);
		pthread_mutex_unlock(&mutexColaNuevos);
		break;
	case 'R':
		log_info(logKernel,"Procesos en estado ---> READY\n");
		pthread_mutex_lock(&mutexColaListos);
		imprimirListadoDeProcesos(colaListos);
		pthread_mutex_unlock(&mutexColaListos);
		break;
	case 'E':
		log_info(logKernel,"Procesos en estado ---> EXEC\n");
		pthread_mutex_lock(&mutexColaEjecucion);
		imprimirListadoDeProcesos(colaEjecucion);
		pthread_mutex_unlock(&mutexColaEjecucion);
		break;
	case 'F':
		log_info(logKernel,"Procesos en estado ---> FINISHED\n");
		pthread_mutex_lock(&mutexColaTerminados);
		imprimirListadoDeProcesos(colaTerminados);
		pthread_mutex_unlock(&mutexColaTerminados);
		break;
	case 'B':
		log_info(logKernel,"Procesos en estado ---> BLOCKED\n");
		pthread_mutex_lock(&mutexColaBloqueados);
		imprimirListadoDeProcesos(colaBloqueados);
		pthread_mutex_unlock(&mutexColaBloqueados);
		break;
	default:
		break;
	}
}

void imprimirListadoDeProcesos(t_list* procesos){
	log_info(logKernel,"Cantidad de procesos: %d\n", procesos->elements_count);
	printf("PID\tCantidad de Rafagas\tCantidad de SysCalls\tPaginas de Heap\t\tCantidad Alocar\tSize Alocar\tCantidad Liberar\tSize Liberar\n");
	int i;
	for(i=0 ; i<procesos->elements_count ; i++){
		t_pcb* proceso= list_get(procesos,i);
		obtenerDatosProceso(proceso->pid);
	}
}



void finalizarProcesoVoluntariamente(int pid){
	pthread_mutex_lock(&mutexNuevoProceso);
	buscarProcesoYTerminarlo(pid);
	pthread_mutex_unlock(&mutexNuevoProceso);
	log_info(logKernel,"Proceso finalizado voluntariamente--->PID: %d",pid);
}



void interfaceTablaGlobalArchivos(){ /*TODO: Mutex tabla global*/
	int i;
	t_entradaTablaGlobal* entrada;
	printf("Direccion\tOpen\n");
	for(i=0;i<tablaArchivosGlobal->elements_count;i++){
		entrada = list_get(tablaArchivosGlobal,i);
		printf("%s\t%d\n",entrada->path,entrada->open);
	}
}


void interfaceModificarGradoMultiprogramacion(){ /*TODO: Ver de dejar cambiar a uno menor*/
	int nuevoGrado;
	pthread_mutex_lock(&mutexNuevoProceso);
	printf("Ingresar nuevo grado de multiprogramacion\n");
	scanf("%d",&nuevoGrado);
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	if(nuevoGrado < gradoMultiProgramacion) {
		log_warning(logKernel,"El valor ingresado es menor a la cantidad de procesos en el sistema actualmente");
		pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
		return;
	}
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);

	pthread_mutex_lock(&mutex_config_gradoMultiProgramacion);
	config_gradoMultiProgramacion= nuevoGrado;
	pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);

	sem_post(&sem_admitirNuevoProceso);
	pthread_mutex_unlock(&mutexNuevoProceso);
	log_info(logKernel,"Se cambio la configuracion del Grado de Multiprogramacion a:%d\n",nuevoGrado);
}

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		logKernel = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		logKernel = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}

void imprimirInterfazUsuario(){

	/**************************************Printea interfaz Usuario Kernel*******************************************************/
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	printf("Para realizar acciones permitidas en la consola Kernel, seleccionar una de las siguientes opciones\n");
	printf("\nIngresar orden de accion:\nO - Obtener datos de proceso\nL - Obtener listado programas\n\tT - Obtener todos los procesos\n\tC - Obtener procesos de un estado\n\t\tN - New\n\t\tR - Ready\n\t\tE - Exec\n\t\tB - Blocked\n\t\tF - Finished\nP - Pausar planificacion\nR - Reanudar planificacion\nG - Mostrar tabla global de archivos\nM - Modif grado multiprogramacion\nK - Finalizar proceso\n");
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	/****************************************************************************************************************************/
}
