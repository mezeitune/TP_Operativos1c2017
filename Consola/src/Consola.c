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
//#include <commons/conexiones.h>
#include "conexiones.h"

int enviarLecturaArchivo(void *ruta, int socket);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void* connectionHandler(int socket);
//void inicializarPID(int pid);
//void enviarPIDAEliminar(int pidAFinalizar,int socket);

t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;
t_list * listaPid;

pthread_t HiloId;

int main(void) {

	leerConfiguracion(
			"/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();
	listaPid = list_create();

	int socketKernel = crear_socket_cliente(ipKernel, puertoKernel);

	int err = pthread_create(&HiloId, NULL, &connectionHandler,
			(void*) socketKernel);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	else
		printf("\n Thread created successfully\n");

	(void) pthread_join(HiloId, NULL);

	return EXIT_SUCCESS;

}

void* connectionHandler(int socket) {

	while (1) {
		char orden;
		char *ruta = (char*) malloc(200 * sizeof(char));
		;


		int *pidAEliminar = (int*) malloc(4 * sizeof(int));
		;

		printf(
				"Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
		scanf(" %c", &orden);
		send(socket, (void*) &orden, sizeof(char), 0);

		switch (orden) {
		case 'I':
			printf(
					"Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
			scanf("%s", ruta);
			if ((enviarLecturaArchivo(ruta, socket)) < 0) {
				printf(
						"La consola se ha desconectado por inconsistencia en el archivo\n");
				exit(1);
			}

			break;
		case 'F':
			printf("Ingresar el PID del programa a finalizar\n");
			scanf("%d", pidAEliminar);
			//enviarPIDAEliminar(pidAEliminar,socket);

			_Bool verificarPid(int pid){
				return pid==pidAEliminar;
			}
			t_list* t_listaaa = list_filter(listaPid,verificarPid);
			printf("%d",list_size(t_listaaa));
			int estaVacia =  list_is_empty(t_listaaa);
			if (estaVacia==0){
				list_remove_by_condition(listaPid, verificarPid);
				printf("El programa AnSISOP de PID : %d  ha finalizado\n",
						*pidAEliminar);
			}else{
				printf("PID incorrecto\n");
			}
			break;
		case 'C':
			system("clear");
			break;
		case 'Q':
			printf("Se ha desconectado el cliente\n");
			exit(1);
			break;
		default:
			printf("ERROR, Orden %c no definida\n", orden);
			break;
		}

	}
}

int enviarLecturaArchivo(void *rut, int socket) {
	FILE *f;
	void *mensaje;
	void *bufferArchivo;
	int tamanioArchivo;
	int pid = 0;
	char *ruta = (char *) rut;

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL) {
		printf("El archivo es inexistente\n");
		return -1;
	}

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo); // Pido memoria para leer el contenido del archivo
	if (bufferArchivo == NULL) {
		fputs("No se pudo conseguir memoria\n", stderr);
		free(bufferArchivo);
		exit(2);
	}

	mensaje = malloc(sizeof(int) * 2 + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {
		fputs("No se pudo conseguir memoria\n", stderr);
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f); // Paso a memoria ( al bufferARchivo) el contenido del archivo.
	//printf("El contenido del archivo que se va a enviar es: \" %s \" \n", bufferArchivo);
	//printf("El tamano del archivo a enviar es: %d\n", tamanioArchivo);

	memcpy(mensaje, &tamanioArchivo, sizeof(int)); // Empaqueto en el mensaje el tamano del archivo a enviar.
	memcpy(mensaje + sizeof(int), bufferArchivo, tamanioArchivo); // Empaqueto en el mensjae, el contenido del archivo.

	send(socket, mensaje, tamanioArchivo + sizeof(int), 0); // Mando el mensjae empaquetado.
	printf("El mensaje ha sido enviado \n");

	recv(socket, &pid, sizeof(int), 0);
	printf("El socket asignado para el proceso iniciado es: %d \n", pid);
	printf("El PID asignado es: %d \n", pid);
	listaPid = list_create();
	list_add(listaPid, &pid);

	free(bufferArchivo);
	free(mensaje);
	return 0;

}

/*void enviarPIDAEliminar(int pidAFinalizar, int socket){

 enviar_string(socket, pidAFinalizar);


 }*/

void leerConfiguracion(char* ruta) {
	configuracion_Consola = config_create(ruta);
	ipKernel = config_get_string_value(configuracion_Consola, "IP_KERNEL");
	puertoKernel = config_get_string_value(configuracion_Consola,
			"PUERTO_KERNEL");
}

void imprimirConfiguraciones() {

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\n", ipKernel,
			puertoKernel);
	printf("---------------------------------------------------\n");
}

