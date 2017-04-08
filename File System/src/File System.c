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

t_config* configuracion_FS;
char* ipKernel;
char* puerto;
char* puntoMontaje;
pthread_t  thread_id;

//the thread function
void *connection_handler(void *);

//--------------------Funciones Conexiones----------------------------//
int crear_socket_servidor(char *ip, char *puerto);
int recibirConexion(int socket_servidor);
void* recibir(int socket);
void enviar(int socket, void* cosaAEnviar, int tamanio);
//----------------------Funciones Conexiones----------------------------//

char nuevaOrdenDeAccion(int puertoCliente);
void leerConfiguracion(char* ruta);

int main(void){
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File System/config_FileSys");

	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\ System/config_FileSys");

	int socket_servidor = crear_socket_servidor(ipKernel,puerto);

		recibirConexion(socket_servidor);
return 0;
}
int crear_socket_servidor(char *ip, char *puerto){
    int descriptorArchivo, estado;
    struct addrinfo hints, *infoServer, *n;

    memset(&hints,0,sizeof (struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((estado = getaddrinfo(ip, puerto, &hints, &infoServer)) != 0){
        fprintf(stderr, "Error en getaddrinfo: %s", gai_strerror(estado));

        return -1;
    }

    for(n = infoServer; n != NULL; n = n->ai_next){
        descriptorArchivo = socket(n->ai_family, n->ai_socktype, n->ai_protocol);
        if(descriptorArchivo != -1) break;
    }

    if(descriptorArchivo == -1){
        perror("Error al crear el socket");
        freeaddrinfo(infoServer);

        return -1;
    }

    int si = 1;

    if(setsockopt(descriptorArchivo,SOL_SOCKET,SO_REUSEADDR,&si,sizeof(int)) == -1){
    	perror("Error en setsockopt");
     //   close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    if (bind(descriptorArchivo, n->ai_addr, n->ai_addrlen) == -1){
    	perror("Error bindeando el socket");
     //   close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    freeaddrinfo(infoServer);

    return descriptorArchivo;
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

char* recibir_string(int socket_aceptado){
	return (char*) recibir(socket_aceptado);
}

void enviar_string(int socket, char* mensaje){
	int tamanio = string_length(mensaje) + 1;

	enviar(socket, (void*) mensaje, tamanio);
}

void enviar(int socket, void* cosaAEnviar, int tamanio){
	void* mensaje = malloc(sizeof(int) + tamanio);
	void* aux = mensaje;
	*((int*)aux) = tamanio;
	aux += sizeof(int);
	memcpy(aux, cosaAEnviar, tamanio);

	send(socket, mensaje, sizeof(int) + tamanio, 0);
	free(mensaje);
}

void* recibir(int socket){
	int checkSocket = -1;

	void* recibido = malloc(sizeof(int));

	checkSocket = read(socket, recibido, sizeof(int));

	int tamanioDelMensaje = *((int*)recibido);

	free(recibido);

	if(!checkSocket) return NULL;

	recibido = malloc(tamanioDelMensaje);

	int bytesRecibidos = 0;

	while(bytesRecibidos < tamanioDelMensaje && checkSocket){
		checkSocket = read(socket, (recibido + bytesRecibidos), (tamanioDelMensaje - bytesRecibidos));
		bytesRecibidos += checkSocket;
	}

	return !checkSocket ? NULL:recibido;
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

	puerto= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
	ipKernel= config_get_string_value(configuracion_FS, "IP_KERNEL");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");

}

