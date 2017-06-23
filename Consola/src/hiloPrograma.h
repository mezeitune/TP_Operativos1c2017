/*
 * hiloPrograma.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "conexiones.h"
#include <time.h>
#include <semaphore.h>


typedef struct {
	int pid;
	struct tm* fechaInicio;
	int cantImpresiones;
	int socketHiloKernel;
	pthread_t idHilo;
}t_hiloPrograma;

pthread_mutex_t mutexListaHilos;
pthread_mutex_t mutex_crearHilo;
pthread_mutex_t mutexRecibirDatos;

sem_t sem_crearHilo;
t_list* listaHilosProgramas;

void crearHiloPrograma();
void* iniciarPrograma(int* socketHiloKernel);
void recibirDatosDelKernel(int socketHiloKernel);
int enviarLecturaArchivo(char *ruta,int socketHiloKernel);
void cargarHiloPrograma(int pid, int socket);
void gestionarCierrePrograma(int pidFinalizar);
void actualizarCantidadImpresiones(int pid);

void crearHiloPrograma(){
	t_hiloPrograma* nuevoPrograma = malloc(sizeof(t_hiloPrograma));
	nuevoPrograma->socketHiloKernel=  crear_socket_cliente(ipKernel,puertoKernel);
	int err = pthread_create(&nuevoPrograma->idHilo , NULL ,(void*)iniciarPrograma ,&nuevoPrograma->socketHiloKernel);
	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));

    time_t tiempoInicio ;
    time(&tiempoInicio);
	nuevoPrograma->fechaInicio=malloc(sizeof(struct tm));
	nuevoPrograma->fechaInicio=localtime(&tiempoInicio);
	nuevoPrograma->cantImpresiones = 0;

	pthread_mutex_lock(&mutexListaHilos);
	list_add(listaHilosProgramas,nuevoPrograma);
	pthread_mutex_unlock(&mutexListaHilos);
}

void* iniciarPrograma(int* socketHiloKernel){
	char *ruta = malloc(200 * sizeof(char));

	printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
	scanf("%s", ruta);

	if ((enviarLecturaArchivo(ruta,*socketHiloKernel)) < 0) {
		log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente");
		/*TODO: Que el socket cierre el hilo*/
		pthread_mutex_unlock(&mutex_crearHilo);
		return -1;
	}
	free(ruta);

	recibirDatosDelKernel(*socketHiloKernel);
	return 0;
}
void finalizarPrograma(){
	char comandoInterruptHandler = 'X';
	char comandoFinalizarPrograma= 'F';
	int procesoATerminar;
	t_hiloPrograma* proceso;
	log_info(loggerConPantalla,"Ingresar el PID del programa a finalizar\n");
	scanf("%d", &procesoATerminar);

		bool verificarPid(t_hiloPrograma* proceso){
			return (proceso->pid == procesoATerminar);
		}

		pthread_mutex_lock(&mutexListaHilos);
		if (list_any_satisfy(listaHilosProgramas,(void*)verificarPid)){

			proceso = list_remove_by_condition(listaHilosProgramas,(void*)verificarPid);
				send(proceso->socketHiloKernel,&comandoInterruptHandler,sizeof(char),0);
				send(proceso->socketHiloKernel,&comandoFinalizarPrograma,sizeof(char),0);
				send(proceso->socketHiloKernel,&procesoATerminar, sizeof(int), 0);
				list_add(listaHilosProgramas,proceso);
			}else	log_error(loggerConPantalla,"\nPID incorrecto\n");

		pthread_mutex_unlock(&mutexListaHilos);

		pthread_mutex_unlock(&mutex_crearHilo);
}


void gestionarCierrePrograma(int pidFinalizar){
	bool verificarPid(t_hiloPrograma* proceso){
		return (proceso->pid == pidFinalizar);
	}
	pthread_mutex_lock(&mutexListaHilos);
	t_hiloPrograma* programaAFinalizar = list_remove_by_condition(listaHilosProgramas,(void*)verificarPid);
	pthread_mutex_unlock(&mutexListaHilos);

	close(programaAFinalizar->socketHiloKernel);

	time_t tiempoFinalizacion;
	time(&tiempoFinalizacion);
	struct tm* fechaFinalizacion = malloc(sizeof(struct tm));
	fechaFinalizacion=localtime(&tiempoFinalizacion);
	double tiempoEjecucion= difftime(mktime(programaAFinalizar->fechaInicio),tiempoFinalizacion);

	printf("Datos de proceso finalizado\n");
	printf("\tHora de inicializacion:%s \n\tHora de finalizacion:%s\n\tTiempo de ejecucion:%e \n\tCantidad de impresiones:%d\n",asctime(programaAFinalizar->fechaInicio),asctime(fechaFinalizacion),tiempoEjecucion,programaAFinalizar->cantImpresiones);

	free(programaAFinalizar);
}


void cargarHiloPrograma(int pid, int socket){

	_Bool verificarSocket(t_hiloPrograma* hiloPrograma){
					return (hiloPrograma->socketHiloKernel ==socket);
					}
	pthread_mutex_lock(&mutexListaHilos);
	t_hiloPrograma* hiloPrograma= list_remove_by_condition(listaHilosProgramas,(void*)verificarSocket);
	hiloPrograma->pid = pid;
	list_add(listaHilosProgramas,hiloPrograma);
	pthread_mutex_unlock(&mutexListaHilos);
}

int enviarLecturaArchivo(char *ruta,int socketHiloKernel) {
	FILE *f;
	void *mensaje;
	char *bufferArchivo;
	int tamanioArchivo;
	char comandoIniciarPrograma='A';

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo);

	if (bufferArchivo == NULL) {
		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria dinamica\n");
		free(bufferArchivo);
		exit(1);
	}

	fread(bufferArchivo, sizeof(char), tamanioArchivo, f);
	mensaje = malloc(sizeof(int) + sizeof(char) + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {
		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	memcpy(mensaje, &comandoIniciarPrograma,sizeof(char));
	memcpy(mensaje + sizeof(char), &tamanioArchivo, sizeof(int));
	memcpy(mensaje + sizeof(char) + sizeof(int), bufferArchivo, tamanioArchivo);
	send(socketHiloKernel, mensaje, tamanioArchivo + sizeof(int) + sizeof(char)  , 0);

	free(bufferArchivo);
	free(mensaje);

	return 0;
}
