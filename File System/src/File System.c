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
#include <commons/conexiones.h>

char *ipKernel;
char *puertoKernel;
char *puntoMontaje;
pthread_t  thread_id;
t_config* configuracion_FS;

//the thread function
void *connection_handler(void *);

int recibirConexion(int socket_servidor);
char nuevaOrdenDeAccion(int puertoCliente);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();

int main(void){
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\ System/config_FileSys");
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\\System/config_FileSys");

	int socket_servidor = crear_socket_servidor("127.0.0.1","5001");

	recibirConexion(socket_servidor);
	return 0;
}

int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 socklen_t addr_size;


	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		printf("Error al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		printf("Se puso el socket en listen\n");
	}

	addr_size = sizeof(their_addr);

	int socket_aceptado;
    while( (socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size)) )
    {
        puts("Connection accepted");

        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &socket_aceptado) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }


	if (socket_aceptado == -1){
		close(socket_servidor);
		printf("Error al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}

char nuevaOrdenDeAccion(int puertoCliente)
{
	char *buffer;
	printf("Esperando Orden del Cliente\n");
	buffer = recibir(puertoCliente);
	//int size_mensaje = sizeof(buffer);
    if(buffer == NULL)
    {
        return 'Q';
    	//puts("Client disconnected");
        //fflush(stdout);
    }
    else if(buffer == NULL)
    {
        return 'X';
    	//perror("recv failed");
    }
    printf("El cliente %d envio el comando:",puertoCliente);
	printf("%c\n",*buffer);
	return *buffer;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char orden = 'F';

	while(orden != 'Q')
	{
		orden = nuevaOrdenDeAccion(sock);
		switch(orden)
		{
		case 'I':
			break;
		case 'S':
			break;
		case 'A':
			break;
		case 'G':
			break;
		case 'F':
			break;
		case 'Q':
			printf("Cliente %d desconectado",sock);
			fflush(stdout);
			break;
		case 'X':
			perror("recv failed");
			break;
		default:
			printf("Error\n");
			break;
		}
	}
    return 0;
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
