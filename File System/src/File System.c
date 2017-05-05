/*
 ============================================================================
 Name        : File.c
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
#include <malloc.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conexiones.h"
#include <commons/log.h>

char *ipKernel;
char *puertoKernel;
char *puntoMontaje;
int contadorConexiones=0;
pthread_t  thread_id;
t_config* configuracion_FS;

//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//

//the thread function
void *connection_handler(void *);

int socket_servidor;

int recibirConexion(int socket_servidor);
char nuevaOrdenDeAccion(int puertoCliente);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connection_handlerR();
int main(void){

	//TODO:
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/'File System'/config_FileSys");
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\\System/config_FileSys");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logFS.txt");

	//socket_servidor = crear_socket_servidor("127.0.0.1","5002");

	//int socket_Kernel = recibirConexion(socket_servidor);

	//connection_Listener(socket_Kernel);
	connection_handlerR();//TODO LO QUE ESTA COMENTADO ARRIBA TENDRIA Q IR DESCOMENTADO , PERO ESTOY USANDO ESTA
				//FUNCION MOMENTANEAMENTE , POR QUE EL KERNEL TODAVIA NO SE COMUNICA CON FS
				//ENTONCES LO HAGO PARA TESTEAR , NO BORRAR LAS OTRAS FUNCIONES QUE HAY


	return 0;
}


char nuevaOrdenDeAccion(int socketCliente)
{
	char* buffer=malloc(sizeof(char));
	char bufferRecibido;

	printf("\n--Esperando una orden del cliente-- \n");

	recv(socketCliente,buffer,sizeof(char),0);
	bufferRecibido = *buffer;

	free(buffer);

    printf("El cliente ha enviado la orden: %c\n",bufferRecibido);
	printf("%c\n",bufferRecibido);
	return bufferRecibido;
}

void connection_Listener(int socket_desc)
{
	int sock;
	//Atiendo al socket del kernel
	if( pthread_create( &thread_id , NULL , connection_handler , (void*) &socket_desc) < 0)
	{
		perror("could not create thread");
	}
	while(1){
		//Quedo a la espera de CPUs y las atiendo
		sock = recibirConexion(socket_servidor);
		if( pthread_create( &thread_id , NULL , connection_handler , (void*) &sock) < 0)
		{
			perror("could not create thread");
		}
	}
}


void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char orden;
    int resultadoDeEjecucion;
	while((orden=nuevaOrdenDeAccion(sock)) != 'Q')
	{
		switch(orden)
		{

		case 'V'://validar archivo
			if( access( "alumno.bin", F_OK ) != -1 ) {
			    // file exists
				printf("el archivo existe");
			} else {
			    // file doesn't exist
				printf("el archivo no existe");
			}
			break;
		case 'C'://crear archivo
			break;
		case 'B'://borrar archivo
			break;
		case 'O'://obtener datos
			break;
		case 'G'://guardar archivo
			break;
		default:
			log_warning(loggerConPantalla,"\nError: Orden %c no definida\n",orden);
			break;
		}
	}
    return 0;
}

void connection_handlerR()
{
    char orden;
    FILE *fp;
    int resultadoDeEjecucion;
    while(orden != 'Q'){

    			printf("\nIngresar orden:\n");
    			scanf(" %c", &orden);

    	switch(orden){
		case 'V'://validar archivo   TERMINADO (FALTA QUE RECIBA EL ARCHIVO QUE SOLICITE DESDE KERNEL)
			if( access( "../metadata/alumno.bin", F_OK ) != -1 ) {
			    // file exists
				printf("el archivo existe");
			} else {
			    // file doesn't exist
				printf("el archivo no existe");
			}
			break;
		case 'C'://crear archivo
			if( access( "../metadata/nuevo.bin", F_OK ) != -1 ) {
				//falta ver en este if de arriba tambien si el archivo existe y si esta en modo "c"
			}else{
				fp = fopen("../metadata/nuevo.bin", "ab+");//creo el archivo
				//falta que por default se le asigne un bloque a ese archivo
			}
			break;
		case 'B'://borrar archivo

			   fp = fopen("../metadata/nuevo.bin", "w");


			   if(remove("../metadata/nuevo.bin") == 0)
			   {
			      printf("File deleted successfully");
			   }
			   else
			   {
			      printf("Error: unable to delete the file");
			   }

			   //falta marcar los bloques como libres dentro del bitmap
			break;
		case 'O'://obtener datos


			break;
		case 'G'://guardar archivo
			break;
		default:
			log_warning(loggerConPantalla,"\nError: Orden %c no definida\n",orden);
			break;
		}
	}
}

void leerConfiguracion(char* ruta){

	puertoKernel= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
	ipKernel= config_get_string_value(configuracion_FS, "IP_KERNEL");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");

}

void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\nPUNTO MONTAJE:%s\n",ipKernel,puertoKernel,puntoMontaje);
		printf("---------------------------------------------------\n");
}



int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 socklen_t addr_size;


	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		log_info(loggerConPantalla,"\nError al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		log_info(loggerConPantalla,"\nSe puso el socket en listen\n");
		printf("---------------------------------------------------\n");
	}

	addr_size = sizeof(their_addr);

	int socket_aceptado;
    socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size);

	contadorConexiones ++;
	printf("\n----------Nueva Conexion aceptada numero: %d ---------\n",contadorConexiones);
	printf("----------Handler asignado a (%d) ---------\n",contadorConexiones);

	if (socket_aceptado == -1){
		close(socket_servidor);
		log_error(loggerConPantalla,"\nError al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}

void inicializarLog(char *rutaDeLog){


		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"FS", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"FS", true, LOG_LEVEL_INFO);

}

