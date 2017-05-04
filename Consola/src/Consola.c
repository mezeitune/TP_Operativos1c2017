/*
 ============================================================================
 Name        : Consola.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
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
#include <pthread.h>
#include <commons/log.h>
#include "conexiones.h"
#include <time.h>
#include <signal.h>
//--------LOG----------------//
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//


int enviarLecturaArchivo(void *ruta, int socket);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void* connectionHandler(int socket);
void imprimirProceso();
void senialImprimir(int n);
t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;

pthread_t HiloId;
t_list * listaPid;
struct tm *tlocal;

typedef struct {
	int pid;
	char fechaInicio;
	int cantImpresiones;
} Pid ;

void cargarPid(Pid* pidEstructura, int pid) {
	pidEstructura->cantImpresiones=0;
	pidEstructura->pid = pid;


}



int main(void) {


	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logConsola.txt");
	signal(SIGUSR1, senialImprimir);
	listaPid = list_create();
	int socketKernel = crear_socket_cliente(ipKernel, puertoKernel);
	int err = pthread_create(&HiloId, NULL, connectionHandler,	(void*) socketKernel);

	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));

	else

	(void) pthread_join(HiloId, NULL);

	return EXIT_SUCCESS;

}

void *connectionHandler(int socket) {

	while (1) {
		char orden;
		char *ruta = (char*) malloc(200 * sizeof(char));;
		int pidAEliminar=0;

		printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
		scanf(" %c", &orden);
		send(socket, (void*) &orden, sizeof(char), 0);

		switch (orden) {
		case 'I':
			printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
			scanf("%s", ruta);
			if ((enviarLecturaArchivo(ruta, socket)) < 0) {

				log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente\n");
			}
			free(ruta);
			break;

		case 'F':

			printf("Ingresar el PID del programa a finalizar\n");
			scanf("%d", &pidAEliminar);


			_Bool verificarPid(Pid* pidNuevoo){
				return (pidNuevoo->pid == pidAEliminar);
				}


			t_list * listaNueva;
			listaNueva= list_create();
			listaNueva= list_filter(listaPid,verificarPid);

			int estaVacia =  list_size(listaNueva);

			if (estaVacia==1){

				list_remove_by_condition(listaPid, verificarPid);
				send(socket, (void*) &pidAEliminar, sizeof(int), 0);

				log_info(loggerConPantalla,"\nEl programa AnSISOP de PID : %d  ha finalizado\n",pidAEliminar);

				Pid* estructuraPidAEliminar=list_get(listaNueva, 0);

				time_t tiempo = time(0);
				struct tm *tlocal = localtime(&tiempo);
				char fechaFin[128];
				strftime(fechaFin,128,"%d/%m/%y %H:%M:%S",tlocal);


				printf("Fecha y Hora de inicializacion : \nFecha y Hora de finalizacion: %s\nCantidad de impresiones: %i\n",fechaFin,estructuraPidAEliminar->cantImpresiones);

			}else{
				log_info(loggerConPantalla,"\nPID incorrecto\n");
			}
			break;

		case 'C':
			system("clear");
			break;

		case 'Q':

			list_destroy_and_destroy_elements(listaPid, free);
			log_warning(loggerConPantalla,"\nSe ha desconectado el cliente\n");

			exit(1);
			break;
		default:
			log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
			break;
		}

	}
}

void senialImprimir(int n){
			if(n == SIGUSR1){
				int bytesARecibir=0;
				int socket=socket;
				char* buffer;
				recv(socket ,&bytesARecibir, sizeof(int),0); //recibo la cantidad de bytes del mensaje del kernel
				buffer = malloc(bytesARecibir); // Pido memoria para recibir el contenido del mensaje con los bytes que recibi antes
				recv(socket,buffer,bytesARecibir ,0);//recibo el mensaje de la kenrel con el tamaño de bytesArecibir

				printf("%s",buffer);
			}
		}


int enviarLecturaArchivo(void *rut, int socket) {
	FILE *f;
	void *mensaje;
	void *bufferArchivo;
	int tamanioArchivo;
	int pid=0;
	char *ruta = (char *) rut;

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo); // Pido memoria para leer el contenido del archivo

	if (bufferArchivo == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(bufferArchivo);
		exit(2);
	}

	mensaje = malloc(sizeof(int) * 2 + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f);

	memcpy(mensaje, &tamanioArchivo, sizeof(int)); // Empaqueto en el mensaje el tamano del archivo a enviar.
	memcpy(mensaje + sizeof(int), bufferArchivo, tamanioArchivo); // Empaqueto en el mensjae, el contenido del archivo.

	send(socket, mensaje, tamanioArchivo + sizeof(int), 0); // Mando el mensjae empaquetado.
	log_info(loggerConPantalla,"\nEl mensaje ha sido enviado al kernel\n");

	recv(socket, &pid, sizeof(int), 0);

	log_info(loggerConPantalla,"\nEl socket asignado para el proceso iniciado es: %d \n", socket);
	log_info(loggerConPantalla,"\nEl PID asignado es: %d \n", pid);

	//creo lista con el pid
	Pid* pidNuevo = malloc(sizeof(pid));
	time_t tiempo = time(NULL);
	struct tm *tlocal = localtime(&tiempo);
	char fechaInicio[128];
	strftime(fechaInicio,128,"%d/%m/%y %H:%M:%S",tlocal);
	cargarPid(pidNuevo,pid);
	list_add(listaPid, pidNuevo);
	//lista con los pid

	free(bufferArchivo);
	free(mensaje);
	return 0;


}
void imprimir(char* mensaje){
	printf("%s\n", mensaje);
}

void leerConfiguracion(char* ruta) {
	configuracion_Consola = config_create(ruta);
	ipKernel = config_get_string_value(configuracion_Consola, "IP_KERNEL");
	puertoKernel = config_get_string_value(configuracion_Consola,"PUERTO_KERNEL");
}

void imprimirConfiguraciones() {

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\n", ipKernel,
			puertoKernel);
	printf("---------------------------------------------------\n");
}


void inicializarLog(char *rutaDeLog){


		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Consola", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Consola", true, LOG_LEVEL_INFO);

}


