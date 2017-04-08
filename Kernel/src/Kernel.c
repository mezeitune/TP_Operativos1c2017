/*
 ============================================================================
 Name        : Kernel.c
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

t_config* configuracion_kernel;

int crear_socket_cliente(char * ip, char * puerto);
void enviar(int socket, void* cosaAEnviar, int tamanio);
void leerConfiguracion(char* ruta);

char *ipMemoria;
char *puertoProg;//2001
char *puertoCPU;//3001
char *puertoMemoria;//4001s
char *ipFileSys;
char *puertoFileSys;
char *quantum;
char *quantumSleep;
char *algoritmo;
char *gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;

int main(void)
{
	char orden;
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");
	int socket_Memoria = crear_socket_cliente("127.0.0.1","4040"); //Variable definidas

	while(1)
		{
			scanf(" %c", &orden);
			enviar(socket_Memoria,(void*) &orden,sizeof(char));
		}
	return 0;
}
int crear_socket_cliente(char * ip, char * puerto){
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

void leerConfiguracion(char* ruta){

	configuracion_kernel = config_create(ruta);
	puertoProg = config_get_int_value(configuracion_kernel,"PUERTO_PROG");
	puertoCPU = config_get_int_value(configuracion_kernel,"IP_CPU");
	ipMemoria = config_get_string_value(configuracion_kernel,"IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel,"IP_FS");
	puertoFileSys = config_get_int_value(configuracion_kernel,"PUERTO_FS");
	quantum = config_get_string_value(configuracion_kernel,"QUANTUM");
	quantumSleep = config_get_int_value(configuracion_kernel,"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion_kernel,"ALGORTIMO");
	gradoMultiProg = config_get_int_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION");
	semIds = config_get_array_value(configuracion_kernel,"SEM_IDS");
	semInit = config_get_array_value(configuracion_kernel,"SEM_INIT");
	sharedVars = config_get_array_value(configuracion_kernel,"SHARED_VARS");

}
