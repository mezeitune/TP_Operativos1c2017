/*
 * hiloPrograma.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "conexiones.h"


typedef struct {
	int pid;
	char* fechaInicio;
	int cantImpresiones;
	int socketHiloKernel;
	pthread_t idHilo;
}t_hiloPrograma;


typedef struct {
	pthread_t idHilo;
}t_hilos;

pthread_mutex_t mutex_crearHilo;
t_list* listaHilosProgramas;

void crearHiloPrograma();
void* iniciarPrograma(int* socketHiloKernel);
void recibirDatosDelKernel(int socketHiloKernel);
int enviarLecturaArchivo(char *ruta,int socketHiloKernel);
void cargarHiloPrograma(int pid, char*tiempo, int socket);
void finalizarPrograma();
void cargarPid(t_hiloPrograma* pidEstructura, int pid,char* fechaActual,int socketHiloKernel);

void crearHiloPrograma(){
	t_hiloPrograma* nuevoPrograma = malloc(sizeof(t_hiloPrograma));
	nuevoPrograma->socketHiloKernel=  crear_socket_cliente(ipKernel,puertoKernel);
	int err = pthread_create(&nuevoPrograma->idHilo , NULL ,(void*)iniciarPrograma ,&nuevoPrograma->socketHiloKernel);
	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));
	list_add(listaHilosProgramas,nuevoPrograma);
}

void* iniciarPrograma(int* socketHiloKernel){
	char *ruta = (char*) malloc(200 * sizeof(char));

	printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
	scanf("%s", ruta);

	if ((enviarLecturaArchivo(ruta,*socketHiloKernel)) < 0) {
		log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente\n");
	}
	free(ruta);

	recibirDatosDelKernel(*socketHiloKernel);

	return 0;
}

void recibirDatosDelKernel(int socketHiloKernel){
	int pid=0;
	int size;
	char* mensaje;

	char *tiempoInicio = malloc(sizeof(char));
	tiempoInicio= temporal_get_string_time();
	recv(socketHiloKernel, &pid, sizeof(int), 0);
	log_info(loggerConPantalla,"\nEl PID asignado es: %d \n", pid);

	cargarHiloPrograma(pid,tiempoInicio,socketHiloKernel);
	/* Se queda escuchando los mensaje para imprimir */
	while(1){
		printf("Hilo programa--- PID: %d ----SOCKET: %d ----- escuchando Kernel\n", pid, socketHiloKernel);
		pthread_mutex_unlock(&mutex_crearHilo);
		recv(socketHiloKernel,&size,sizeof(int),0);
		mensaje = malloc(size);
		printf("El tamano del mensaje a recibir es: %d\n", size);
		recv(socketHiloKernel,(void*)mensaje,size,0);
		printf("%s\n",mensaje);
	}

	printf("Termine de recibir los datos del kernel\n");
}


void cargarHiloPrograma(int pid, char*tiempo, int socket){

	_Bool verificarSocket(t_hiloPrograma* hiloPrograma){
					return (hiloPrograma->socketHiloKernel ==socket);
					}
	t_hiloPrograma* hiloPrograma= list_find(listaHilosProgramas ,verificarSocket);
	hiloPrograma->pid = pid;
	hiloPrograma->fechaInicio = tiempo;
	list_remove_by_condition(listaHilosProgramas, verificarSocket);
	list_add(listaHilosProgramas,hiloPrograma);
}
void finalizarPrograma(){
	t_hilos * hiloACerrar = malloc(sizeof(t_hilos));
	int pidAEliminar;
	log_info(loggerConPantalla,"Ingresar el PID del programa a finalizar\n");
	scanf("%d", &pidAEliminar);
		_Bool verificarPid(t_hiloPrograma* pidDeLaLista){
			return (pidDeLaLista->pid == pidAEliminar);
		}
	t_list * listaNueva;
	listaNueva= list_create();
	listaNueva= list_filter(listaHilosProgramas,verificarPid);
	int estaVacia =  list_size(listaNueva);
		if (estaVacia==1){
				list_remove_by_condition(listaHilosProgramas, verificarPid);
				send(socketKernel, (void*) &pidAEliminar, sizeof(int), 0);
				printf("----------------------------------------------------------------------\n");
				log_info(loggerConPantalla,"\nEl programa AnSISOP de PID : %d  ha finalizado",pidAEliminar);
				t_hiloPrograma* estructuraPidAEliminar=list_get(listaNueva, 0);
				char *fechaActual = malloc(sizeof(char));
				fechaActual= temporal_get_string_time();
				//int tiempoEjecucion= obtenerTiempoEjecucion(estructuraPidAEliminar->fechaInicio,fechaActual);
				//aca falta la resta de fechas e informarlas
				printf("----------------------------------------------------------------------\n");
				printf("Hora de inicializacion : %s \n Hora de finalizacion: %s\nTiempo de ejecucion: \nCantidad de impresiones: %i\n",estructuraPidAEliminar->fechaInicio,fechaActual,estructuraPidAEliminar->cantImpresiones);
				printf("----------------------------------------------------------------------\n");
				pthread_join(hiloACerrar->idHilo, NULL);
				log_info(loggerSinPantalla,"El hilo ha finalizado con exito");
				free(fechaActual);
			}else{
						log_info(loggerConPantalla,"\nPID incorrecto\n");
			}
		free(hiloACerrar);
}
int enviarLecturaArchivo(char *ruta,int socketHiloKernel) {
	FILE *f;
	void *mensaje;
	char *bufferArchivo;
	int tamanioArchivo;
	char comandoIniciarPrograma='I';

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo + sizeof(char)); // Pido memoria para leer el contenido del archivo

	if (bufferArchivo == NULL) {
		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria dinamica\n");
		free(bufferArchivo);
		exit(2);
	}

	mensaje = malloc(sizeof(int) + sizeof(char) + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f);
	strcat(bufferArchivo,"\0");
	tamanioArchivo += sizeof(char);

	printf("El tamano del archivo a enviar es: %d\n", tamanioArchivo);
	printf("El archivo a enviar es:\n %s\n", bufferArchivo);

	memcpy(mensaje, &comandoIniciarPrograma,sizeof(char));
	memcpy(mensaje + sizeof(char), &tamanioArchivo, sizeof(int));
	memcpy(mensaje + sizeof(char) + sizeof(int), bufferArchivo, tamanioArchivo);
	printf("Enviando codigo ANSISOP por socket: %d\n", socketHiloKernel);
	send(socketHiloKernel, mensaje, tamanioArchivo + sizeof(int) + sizeof(char)  , 0);
	log_info(loggerConPantalla,"\nEl programa ANSISOP ha sido enviado al Kernel\n");

	free(bufferArchivo);
	free(mensaje);

	return 0;
}


void cargarPid(t_hiloPrograma* pidEstructura, int pid,char* fechaActual,int socketHiloKernel) {
	pidEstructura->cantImpresiones=0;
	pidEstructura->pid = pid;
	pidEstructura->fechaInicio=fechaActual;
	pidEstructura->socketHiloKernel=socketHiloKernel;
	list_add(listaPid, pidEstructura);
}
