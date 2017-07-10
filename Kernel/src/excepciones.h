/*
 * Excepciones.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef EXCEPCIONES_H_
#define EXCEPCIONES_H_
#define CANTIDADEXCEPCIONES 20
#include "conexionConsola.h"
#include "sockets.h"
#include "listasAdministrativas.h"
#include "sincronizacion.h"
#include "capaFilesystem.h"
#include "logs.h"

typedef struct{
	int value;
	char* mensaje;
}t_exitCode;

enum {
	EXIT_OK,
	EXIT_RESOURCE,
	EXIT_FILE_NOT_FOUND,
	EXIT_READ_PERMISSIONS,
	EXIT_WRITE_PERMISSIONS,
	EXIT_CREATE_PERMISSIONS,
	EXIT_MEMORY_EXCEPTION,
	EXIT_DISCONNECTED_CONSOLE,
	EXIT_END_OF_PROCESS,
	EXIT_PAGE_OVERSIZE,
	EXIT_PAGE_LIMIT,
	EXIT_STACKOVERFLOW,
	EXIT_FILE_CANNOT_BE_DELETE,
	EXIT_FILE_DESCRIPTOR_NOT_OPEN,
	EXIT_DIDNOT_OPEN_TABLE,
	EXIT_FILESYSTEM_EXCEPTION
};

int resultadoEjecucion=-1;

t_exitCode* exitCodeArray [CANTIDADEXCEPCIONES];
void inicializarExitCodeArray();

/*Rutinas para finalizar un proceso*/
void expropiarVoluntariamente(int socket);
t_pcb* expropiarPorEjecucion(int socket);

void cambiarEstadoCpu(int socket,int estado);
void removerDeColaEjecucion(int pid);

void finalizarHiloPrograma(int pid);
void liberarRecursosEnMemoria(t_pcb* pcbProcesoTerminado);
void liberarMemoriaDinamica(int pid);
void cambiarEstadoATerminado(t_pcb* procesoTerminar);
void verificarArchivosAbiertos(int pid);

void encolarEnListaParaTerminar(t_pcb* proceso);


void excepcionPlanificacionDetenida(int socket);

/*FileSystem*/
void excepecionFileSystem(int socket,int pid);
void excepcionPermisosEscritura(int socket,int pid);
void excepcionPermisosLectura(int socket,int pid);
void excepcionPermisosCrear(int socket,int pid);
void excepcionArchivoInexistente(int socket,int pid);
void excepcionNoPudoBorrarArchivo(int socket,int pid);
void excepcionFileDescriptorNoAbierto(int socket,int pid);
void excepcionSinTablaArchivos(int socket,int pid);

/*Memoria*/
void excepcionReservaRecursos(int socketAceptado,t_pcb* pcb);
void excepcionPageSizeLimit(int socket,int pid);
void excepcionCantidadDePaginas(int socket,int pid);
void excepcionStackOverflow(int socket);


void excepcionPlanificacionDetenida(int socket){
	log_error(logKernel,"Informando a Consola excepcion por planificacion detenido");
	informarConsola(socket,exitCodeArray[EXIT_RESOURCE]->mensaje,strlen(exitCodeArray[EXIT_RESOURCE]->mensaje));
	char* mensaje = "Finalizar";
	int size=strlen(mensaje);
	informarConsola(socket,mensaje,size);
	eliminarSocket(socket);

}

/*
 * Excepeciones de FileSystem.
 */
void excepcionFileSystem(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion de fileSystem");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILESYSTEM_EXCEPTION]->mensaje,strlen(exitCodeArray[EXIT_FILESYSTEM_EXCEPTION]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_FILESYSTEM_EXCEPTION]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionPermisosEscritura(int socketCPU,int pid){
	log_error(logKernel,"Informando a Consola excepcion por permisos de escritura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socketCPU);
	proceso->exitCode = exitCodeArray[EXIT_WRITE_PERMISSIONS]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionNoPudoBorrarArchivo(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion por no poder borrar archivo");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILE_CANNOT_BE_DELETE]->mensaje,strlen(exitCodeArray[EXIT_FILE_CANNOT_BE_DELETE]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_FILE_CANNOT_BE_DELETE]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionFileDescriptorNoAbierto(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion por no existir el fd indicado");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILE_DESCRIPTOR_NOT_OPEN]->mensaje,strlen(exitCodeArray[EXIT_FILE_DESCRIPTOR_NOT_OPEN]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_FILE_DESCRIPTOR_NOT_OPEN]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionSinTablaArchivos(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion porque el proceso nunca inicializo la tabla de archivos");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_DIDNOT_OPEN_TABLE]->mensaje,strlen(exitCodeArray[EXIT_DIDNOT_OPEN_TABLE]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_DIDNOT_OPEN_TABLE]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionPermisosLectura(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion por permisos de lectura");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_READ_PERMISSIONS]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionPermisosCrear(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion por permisos de creacion");
	t_pcb* proceso = expropiarPorEjecucion(socket);
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje,strlen(exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje));
	proceso->exitCode = exitCodeArray[EXIT_CREATE_PERMISSIONS]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionArchivoInexistente(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepcion por archivo inexistente");
	t_pcb* proceso = expropiarPorEjecucion(socket);
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje,strlen(exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje));
	proceso->exitCode = exitCodeArray[EXIT_FILE_NOT_FOUND]->value;
	encolarEnListaParaTerminar(proceso);
}


/*
 * Excepeciones Memoria
 */
void excepcionReservaRecursos(int socket,t_pcb* proceso){
	log_error(logKernel,"Informando a Consola excepcion por problemas al reservar recursos");
	informarConsola(socket,exitCodeArray[EXIT_RESOURCE]->mensaje,strlen(exitCodeArray[EXIT_RESOURCE]->mensaje));
	proceso->exitCode = exitCodeArray[EXIT_RESOURCE]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionPageSizeLimit(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepecion de exceso de memoria dinamica");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje,strlen(exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_PAGE_OVERSIZE]->value;
	encolarEnListaParaTerminar(proceso);
}

void excepcionCantidadDePaginas(int socket,int pid){
	log_error(logKernel,"Informando a Consola excepecion de exceso de paginas");
	informarConsola(buscarSocketHiloPrograma(pid),exitCodeArray[EXIT_PAGE_LIMIT]->mensaje,strlen(exitCodeArray[EXIT_PAGE_LIMIT]->mensaje));
	t_pcb* proceso = expropiarPorEjecucion(socket);
	proceso->exitCode = exitCodeArray[EXIT_PAGE_LIMIT]->value;
	encolarEnListaParaTerminar(proceso);
}

/*TODO: Dos excepeciones iguales?*/
void excepcionStackOverflow(int socket){

	int cantidadDeRafagas;

	log_error(logKernel,"Informando a Consola excepcion por StackOverflow");
	t_pcb* proceso=recibirYDeserializarPcb(socket);
	recv(socket,&cantidadDeRafagas,sizeof(int),0);

	actualizarRafagas(proceso->pid,cantidadDeRafagas);

	informarConsola(buscarSocketHiloPrograma(proceso->pid),exitCodeArray[EXIT_STACKOVERFLOW]->mensaje,strlen(exitCodeArray[EXIT_STACKOVERFLOW]->mensaje));
	proceso->exitCode =  exitCodeArray[EXIT_STACKOVERFLOW]->value;
	removerDeColaEjecucion(proceso->pid);
	encolarEnListaParaTerminar(proceso);
	cambiarEstadoCpu(socket,OCIOSA);


	sem_post(&sem_CPU);
}

void excepcionDireccionInvalida(int socket){
	int cantidadDeRafagas;
	log_error(logKernel,"Informando a Consola excepcion por Direccion Invalida de Memoria pedido por un proceso ANSISOP");
	t_pcb* proceso=recibirYDeserializarPcb(socket);
	recv(socket,&cantidadDeRafagas,sizeof(int),0);

	actualizarRafagas(proceso->pid,cantidadDeRafagas);

	informarConsola(buscarSocketHiloPrograma(proceso->pid),exitCodeArray[EXIT_STACKOVERFLOW]->mensaje,strlen(exitCodeArray[EXIT_STACKOVERFLOW]->mensaje));
	proceso->exitCode =  exitCodeArray[EXIT_STACKOVERFLOW]->value;
	removerDeColaEjecucion(proceso->pid);
	encolarEnListaParaTerminar(proceso);
	cambiarEstadoCpu(socket,OCIOSA);
	sem_post(&sem_CPU);
}

/*
 * Rutinas para finalizar un proceso
 */

void expropiarVoluntariamente(int socket){
	log_info(logKernelPantalla,"Expropiando proceso--->CPU:%d",socket);

	_Bool verificaSocket(t_cpu* cpu){
		return cpu->socket==socket;
	}

	pthread_mutex_lock(&mutexListaCPU);
	t_cpu* cpu=list_find(listaCPU,(void*)verificaSocket);
	pthread_mutex_unlock(&mutexListaCPU);

	int *pid = malloc(sizeof(int));
	*pid = cpu->pid;

	list_add(listaProcesosInterrumpidos,pid);
}



t_pcb* expropiarPorEjecucion(int socket){
	t_pcb* pcb;
	int resultadoEjecucion=-1;
	int rafagas;
	log_info(logKernel,"Expropiando proceso--->CPU:%d",socket);

		send(socket,&resultadoEjecucion,sizeof(int),0);

		pcb = recibirYDeserializarPcb(socket);
		recv(socket,&rafagas,sizeof(int),0);
		actualizarRafagas(pcb->pid,rafagas);
		removerDeColaEjecucion(pcb->pid);
		cambiarEstadoCpu(socket,OCIOSA);
		sem_post(&sem_CPU);
		return pcb;
}

void cambiarEstadoATerminado(t_pcb* procesoTerminar){
	log_info(logKernel,"Almacenando en Terminados--->PID:%d",procesoTerminar->pid);
	_Bool verificaPid(t_pcb* pcb){
			return (pcb->pid == procesoTerminar->pid);
		}

	pthread_mutex_lock(&mutexColaTerminados);
	list_add(colaTerminados,procesoTerminar);
	pthread_mutex_unlock(&mutexColaTerminados);
}

void finalizarHiloPrograma(int pid){
	int hiloNoFinalizado;
	int size=sizeof(char)* strlen("Finalizar");
	char* mensaje = malloc(size * sizeof(char));
	t_consola* consola = malloc(sizeof(t_consola));
	mensaje = "Finalizar";
	_Bool verificaPid(t_consola* consolathread){
			return (consolathread->pid == pid);
	}
		pthread_mutex_lock(&mutexListaConsolas);
		hiloNoFinalizado = list_any_satisfy(listaConsolas,(void*)verificaPid);

		if(hiloNoFinalizado){

		consola = list_remove_by_condition(listaConsolas,(void*)verificaPid);

		informarConsola(consola->socketHiloPrograma,mensaje,size);
		eliminarSocket(consola->socketHiloPrograma);
		free(consola);
		}
		pthread_mutex_unlock(&mutexListaConsolas);

	//free(mensaje);TODO: Ver este free
}

void cambiarEstadoCpu(int socket,int estado){
	_Bool verificaSocket(t_cpu* cpu){
		return cpu->socket == socket;
	}
	pthread_mutex_lock(&mutexListaCPU);
	t_cpu* cpu = list_remove_by_condition(listaCPU,(void*)verificaSocket);
	cpu->estado = estado;
	list_add(listaCPU,cpu);
	pthread_mutex_unlock(&mutexListaCPU);

}

void removerDeColaEjecucion(int pid){
	log_info(logKernelPantalla,"Removiendo proceso de cola de ejecucion--->PID:%d\n",pid);
	_Bool verificaPid(t_pcb* proceso){
			return (proceso->pid == pid);
		}
	pthread_mutex_lock(&mutexColaEjecucion);
	list_remove_and_destroy_by_condition(colaEjecucion, (void*)verificaPid,free);
	pthread_mutex_unlock(&mutexColaEjecucion);
}

void encolarEnListaParaTerminar(t_pcb* proceso){
	pthread_mutex_lock(&mutexListaEspera);
	list_add(listaEspera,proceso);
	pthread_mutex_unlock(&mutexListaEspera);

	sem_post(&sem_administrarFinProceso);
}

/*
void verificarArchivosAbiertos(int pid){
	printf("Verificando archivos abiertos\n");
	int i;
	_Bool verificaPid(t_indiceTablaProceso* indice){
		return indice->pid == pid;
	}

	t_indiceTablaProceso* indice = list_find(listaTablasProcesos,(void*)verificaPid);


	for(i=0;i<indice->tablaProceso->elements_count;i++){
		t_entradaTablaProceso* entrada= list_get(indice->tablaProceso,i);
		disminuirOpenYVerificarExistenciaEntradaGlobal(entrada->globalFd);
	}
	printf("Termine de verificar archivos abiertos\n");
}
*/

void inicializarExitCodeArray(){
	int i;
	for(i = 0 ; i< CANTIDADEXCEPCIONES ; i++){
		exitCodeArray [i] = malloc (sizeof(t_exitCode));
	}

	exitCodeArray[EXIT_OK]->value = 0;
	exitCodeArray[EXIT_OK]->mensaje= "Programa finalizado exitosamente";

	exitCodeArray[EXIT_RESOURCE]->value = -1;
	exitCodeArray[EXIT_RESOURCE]->mensaje= "No se pudieron reservar recursos para ejecutar el programa";

	exitCodeArray[EXIT_FILE_NOT_FOUND]->value = -2;
	exitCodeArray[EXIT_FILE_NOT_FOUND]->mensaje= "El programa intento acceder a un archivo que no existe";

	exitCodeArray[EXIT_READ_PERMISSIONS]->value = -3;
	exitCodeArray[EXIT_READ_PERMISSIONS]->mensaje= "El programa intento leer un archivo sin permisos";

	exitCodeArray[EXIT_WRITE_PERMISSIONS]->value = -4;
	exitCodeArray[EXIT_WRITE_PERMISSIONS]->mensaje= "El programa intento escribir un archivo sin permisos";

	exitCodeArray[EXIT_CREATE_PERMISSIONS]->value = -5;
	exitCodeArray[EXIT_CREATE_PERMISSIONS]->mensaje= "El programa intento crear un archivo sin permisos";


	exitCodeArray[EXIT_MEMORY_EXCEPTION]->value = -6;
	exitCodeArray[EXIT_MEMORY_EXCEPTION]->mensaje= "Excepcion de memoria";

	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->value = -7;
	exitCodeArray[EXIT_DISCONNECTED_CONSOLE]->mensaje= "Finalizado a traves de desconexion de consola";

	exitCodeArray[EXIT_END_OF_PROCESS]->value = -8;
	exitCodeArray[EXIT_END_OF_PROCESS]->mensaje= "Finalizado a traves del comando Finalizar Programa";

	exitCodeArray[EXIT_PAGE_OVERSIZE]->value = -9;
	exitCodeArray[EXIT_PAGE_OVERSIZE]->mensaje= "Se intento reservar mas memoria que el tamano de una pagina";

	exitCodeArray[EXIT_PAGE_LIMIT]->value = -10;
	exitCodeArray[EXIT_PAGE_LIMIT]->mensaje= "No se pueden asignar mas paginas al proceso";

	exitCodeArray[EXIT_STACKOVERFLOW]->value = -11;
	exitCodeArray[EXIT_STACKOVERFLOW]->mensaje= "El programa sufrio StackOverflow";


	exitCodeArray[EXIT_FILE_CANNOT_BE_DELETE]->value = -12;
	exitCodeArray[EXIT_FILE_CANNOT_BE_DELETE]->mensaje = "El archivo no pudo ser borrado porque otro proceso lo tiene abierto";

	exitCodeArray[EXIT_FILE_DESCRIPTOR_NOT_OPEN]->value = -13;
	exitCodeArray[EXIT_FILE_DESCRIPTOR_NOT_OPEN]->mensaje = "El archivo nunca abrio  el file descriptor indicado";

	exitCodeArray[EXIT_DIDNOT_OPEN_TABLE]->value = -14;
	exitCodeArray[EXIT_DIDNOT_OPEN_TABLE]->mensaje = "El proceso nunca inicializo su tabla de archivos";

	exitCodeArray[EXIT_FILESYSTEM_EXCEPTION]->value = -15;
	exitCodeArray[EXIT_FILESYSTEM_EXCEPTION]->mensaje="Ha surgido una excepecion de Filesystem";



}
#endif /* EXCEPCIONES_H_ */
