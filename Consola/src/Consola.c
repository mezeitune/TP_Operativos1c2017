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
#include <pthread.h>
#include <commons/conexiones.h>

void enviarLecturaArchivo(void *ruta, int socket);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socket);
//void inicializarPID(int pid);
//void enviarPIDAEliminar(int pidAFinalizar,int socket);

t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;
pthread_t thread_id;

// /home/utnso/Escritorio/archivoPrueba

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();


	char *ruta = (char*) malloc(200*sizeof(char));

	int socketKernel = crear_socket_cliente(ipKernel, puertoKernel);

	connectionHandler(socketKernel);

	free(ruta);
	return EXIT_SUCCESS;
}




void connectionHandler(int socket){

	char orden;
	char *ruta = (char*) malloc(200*sizeof(char));;
	int pid;
	int *pidAEliminar= (int*) malloc(4*sizeof(int));;

	while(1){
		while(orden != 'Q'){

			printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
			scanf(" %c", &orden);
			send(socket, (void*)&orden, sizeof(char),0);

			switch(orden){
			case 'I':
					printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
					scanf("%s", ruta);
					enviarLecturaArchivo(ruta, socket);
					pid=0;

					break;
			case 'F':
					printf("Ingresar el PID del programa a finalizar\n");
					scanf("%d", pidAEliminar);
					//enviarPIDAEliminar(pidAEliminar,socket);
						if(pid == *pidAEliminar)
							printf("El programa AnSISOP de PID : %d  ha finalizado\n",*pidAEliminar);
								else
									printf("PID incorrecto\n");
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
					exit(1);
					break;
			}
		}
			orden = '\0';

	}
}






void enviarLecturaArchivo(void *rut, int socket){

	FILE *f;
	char *buffer;
	unsigned int tamanioArchivo;
	char *ruta = (char *)rut;

	if ((f = fopen(ruta, "r+")) == NULL) {
			fputs("Archivo inexistente\n", stderr);
			exit(1);
		}

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	buffer = (char*) malloc(sizeof(char) * tamanioArchivo);


	if (buffer == NULL) {
		fputs("No se pudo conseguir memoria\n", stderr);
		free(buffer);
		exit(2);
	}

	fread(buffer, sizeof(buffer), tamanioArchivo, f);

	//printf("%s",buffer);
	enviar_string(socket, (void*)buffer);

	free(buffer);

}

/*void enviarPIDAEliminar(int pidAFinalizar, int socket){

	enviar_string(socket, pidAFinalizar);


}*/




void leerConfiguracion(char* ruta) {
	configuracion_Consola = config_create(ruta);
	ipKernel = config_get_string_value(configuracion_Consola,"IP_KERNEL");
	puertoKernel = config_get_string_value(configuracion_Consola,"PUERTO_KERNEL");
}

void imprimirConfiguraciones(){

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\n",ipKernel,puertoKernel);
	printf("---------------------------------------------------\n");
}

