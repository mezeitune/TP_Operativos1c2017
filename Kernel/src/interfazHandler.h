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
#include "logs.h"

void interfazHandler();
void imprimirInterfazUsuario();

void interfaceObtenerListadoProcesos();
void interfaceObtenerDatosProceso();
void interfaceFinalizarProcesoVoluntariamente();
void interfaceTablaGlobalArchivos();
void interfaceSolicitarContenidoMemoria();
void interfaceModificarGradoMultiprogramacion();
void interfaceMostrarGradoMultiprogramacion();

void finalizarProcesoVoluntariamente(int pid,int exitCode);
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


void interfazHandler(){
	char orden;
	int cont = 0;
	do{
		scanf("%c",&orden);
		cont++;
		switch(orden){
				case 'L': 	interfaceObtenerListadoProcesos(); /*TODO: Solucionar el asunto de como se imprime todo*/
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
				case 'E': 	interfaceMostrarGradoMultiprogramacion();
							break;
				case 'S':	interfaceSolicitarContenidoMemoria();
							break;
				case 'I':	imprimirInterfazUsuario();
							break;
				case 'F':
						testEscribirArchivo();
					//interfazHandlerParaFileSystem('A',1);
					break;
				case 'H': imprimirListaAdministrativaHeap();
						  break;
				case 'Z':
						testLeerArchivo();
					break;
				case 'X':
						testBorrarArchivo();
					break;
				case 'D': sem_post(&sem_planificacion);
						  break;
				default:
					if(cont!=2){} else cont = 0;
					break;
		}
	}while(1);

}

void interfaceMostrarGradoMultiprogramacion(){

	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	printf("\033[22;34mEl grado de multiprogramacion actual es:%d\033[0m\n",gradoMultiProgramacion);
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
}

void interfaceObtenerDatosProceso(){
	printf("Ingrese el pid del proceso\n");
	scanf("%d",&pid);
		if(verificarProcesoExistente(pid)<0){
			log_warning(logKernelPantalla,"Proceso no existente---> PID: %d", pid);
			return;
		}
		printf("\033[22;34mDatos del proceso--->%d\033[0m\n",pid);
		printf("\033[22;34mPID/RAFAGAS/SysCalls/Pags. Heap/Cant. Alocar/ Size Alocar/ Cant Liberar/ Size Liberar/\033[0m\n");
		obtenerDatosProceso(pid);
}

void interfaceFinalizarProcesoVoluntariamente(){
	printf("Ingrese el pid del proceso a finalizar\n");
	scanf("%d",&pid);
			if(verificarProcesoExistente(pid)<0){
				log_warning(logKernelPantalla,"Proceso no existente--->PID:%d\n",pid);
				return;
				}
			if(verificarProcesoNoTerminado(pid)<0){
				log_info(logKernel,"El proceso ya ha finalizado--->PID:%d\n",pid);
				return;
				}
	finalizarProcesoVoluntariamente(pid,exitCodeArray[EXIT_END_OF_PROCESS]->value);
}

void interfaceSolicitarContenidoMemoria(){
	char* mensaje;
	if((solicitarContenidoAMemoria(&mensaje))<0){
		log_error(logKernelPantalla,"No se pudo solicitar el contenido\n");
			return;
		}
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
	int existe=-1;
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

void obtenerDatosProceso(int pid){
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
	printf("%d\t%d\t%d\t  %d\t      %d\t\t   %d\t\t%d\t\t%d\n",pid,proceso->cantRafagas,proceso->cantSysCalls,proceso->cantPaginasHeap,proceso->cantAlocar,
				proceso->sizeAlocar,proceso->cantLiberar,proceso->sizeLiberar);
}

void imprimirTablaArchivosProceso(int pid){

	_Bool verificaPidArchivo(t_indiceTablaProceso* entrada){
		return entrada->pid == pid;
	}
	int i;
	printf("\t\t\tTabla de archivos del proceso\n");
	printf("\t\t\tFile Descriptor\tFlags\tIndice Global\tCursor\n");

	t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPidArchivo);
	t_entradaTablaProceso* entrada;

	for(i=0;i<entradaTablaProceso->tablaProceso->elements_count;i++){
		entrada = list_get(entradaTablaProceso->tablaProceso,i);
		log_info(logKernelPantalla,"\t\t\t\t%d\t%s\t\t%d\t%d\n",entrada->fd,entrada->flags,entrada->indiceGlobal,entrada->puntero);
	}
	list_add(listaTablasProcesos,entradaTablaProceso);
}

void interfaceObtenerListadoProcesos(){
	char ordenMenu;
	char ordenSubMenu;
	//printf("T:Mostrar todos los procesos\nC:Mostrar una cola\n");
	scanf("%c",&ordenMenu);
	switch(ordenMenu){
	case 'T':
		mostrarTodosLosProcesos();
		break;
	case 'C':
		scanf("%c",&ordenSubMenu);
		mostrarProcesos(ordenSubMenu);
		break;
	default:
		log_error(logKernel,"Orden no reconocida-->ORDEN: %c\n", ordenMenu);
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

void mostrarProcesos(char orden){

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



void finalizarProcesoVoluntariamente(int pid,int exitCode){
	pthread_mutex_lock(&mutexNuevoProceso);
	buscarProcesoYTerminarlo(pid,exitCode);
	pthread_mutex_unlock(&mutexNuevoProceso);
}



void interfaceTablaGlobalArchivos(){
	int i;
	t_entradaTablaGlobal* entrada;
	printf("\t\tTabla Global de Archivos\n");
	printf("\tDireccion\tAperturas\n");
	for(i=0;i<tablaArchivosGlobal->elements_count;i++){
		entrada = list_get(tablaArchivosGlobal,i);
		log_info(logKernelPantalla,"\t%s\t%d\n",entrada->path,entrada->open);
	}
}


void interfaceModificarGradoMultiprogramacion(){ /*TODO: Ver de dejar cambiar a uno menor*/
	int nuevoGrado;
	pthread_mutex_lock(&mutexNuevoProceso);

	printf("Ingresar nuevo grado de multiprogramacion\n");
	scanf("%d",&nuevoGrado);

	/*if(nuevoGrado < gradoMultiProgramacion) {
		log_warning(logKernel,"El valor ingresado es menor a la cantidad de procesos en el sistema actualmente");
	}
	*/

	pthread_mutex_lock(&mutex_config_gradoMultiProgramacion);
	config_gradoMultiProgramacion= nuevoGrado;
	pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);

	if(nuevoGrado > gradoMultiProgramacion)sem_post(&sem_admitirNuevoProceso);
	pthread_mutex_unlock(&mutexNuevoProceso);
	log_info(logKernel,"Se cambio la configuracion del Grado de Multiprogramacion a:%d\n",nuevoGrado);
}


void imprimirInterfazUsuario(){

	/**************************************Printea interfaz Usuario Kernel*******************************************************/
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	printf("Para realizar acciones permitidas en la consola Kernel, seleccionar una de las siguientes opciones\n");
	printf("\nIngresar orden de accion:\n"
			"O - Obtener datos de proceso\n"
			"L - Obtener listado programas\n"
					"\tT - Obtener todos los procesos\n"
					"\tC - Obtener procesos de un estado\n"
							"\t\tN - New\n"
							"\t\tR - Ready\n"
							"\t\tE - Execution\n"
							"\t\tB - Blocked\n"
							"\t\tF - Finished\n"
			"P - Pausar planificacion\n"
			"R - Reanudar planificacion\n"
			"G - Mostrar tabla global de archivos\n"
			"M - Modif grado multiprogramacion\n"
			"E - Ver grado multiprogramacion\n"
			"K - Finalizar proceso\n"
			"H - Mostrar estructura Heap");
	printf("\n-----------------------------------------------------------------------------------------------------\n");
	/****************************************************************************************************************************/
}
