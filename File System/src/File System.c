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


t_config* configuracion_FS;
char *ipKernel;
char *puerto;
char *puntoMontaje;


//------------------Configs----------------------//
int crear_socket_cliente(char * ip, char * puerto);
void enviar(int socket, void* cosaAEnviar, int tamanio);
void leerConfiguracion(char* ruta);
//-----------------------------------------------//


int main(void){
	char orden;
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\ System/config_FileSys");

	int socket_Kernel = crear_socket_cliente("127.0.0.1","5002"); //Variable definidas
		while(1)
			{
				scanf("%c", &orden);
				enviar(socket_Kernel,(void*) &orden,sizeof(char));
			}
		return 0;
	}

int crear_socket_cliente(char * ip, char * puerto){
    int descriptorArchivo, estado;
    struct addrinfo hints, *infoServer, *n;

	//printf("CONFIGURACIONES\nPuerto=%s\nPunto Montaje=%s\n",puerto,puntoMontaje);

    memset(&hints,0,sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((estado = getaddrinfo(ip, puerto, &hints, &infoServer)) != 0){
        fprintf(stderr, "Error en getaddrinfo: %s", gai_strerror(estado));
        return -1;
    }

    for(n = infoServer; n != NULL; n = n->ai_next){
        descriptorArchivo = socket(n->ai_family, n->ai_socktype, n->ai_protocol);
        if(descriptorArchivo != -1)
            break;
    }

    if(descriptorArchivo == -1){
        perror("Error al crear el socket");
        freeaddrinfo(infoServer);
        return -1;
    }

    estado = connect(descriptorArchivo, n->ai_addr, n->ai_addrlen);


    if (estado == -1){
        perror("Error conectando el socket");
        freeaddrinfo(infoServer);
        return -1;
    }

	puerto = config_get_string_value(configuracion_FS,"PUERTO");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");

    freeaddrinfo(infoServer);

    return descriptorArchivo;
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
void leerConfiguracion(char* ruta){

	puerto = config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
	ipKernel= config_get_string_value(configuracion_FS, "IP_KERNEL");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");

}

