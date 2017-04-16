/*
 ============================================================================
 Name        : CPU.c
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
#include <commons/conexiones.h>

void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();

t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();

	char orden='A';
	int socket_Kernel = crear_socket_cliente(ipKernel,puertoKernel);

while(1){

	printf("Ingrese Orden\n");
		scanf(" %c", &orden);
		enviar_string(socket_Kernel,&orden);

	if(orden == 'Q') printf("Se ha sacado a un cliente del modulo Connection Handler\nAhora ya puede apretar CTRL + C si desea desconectar al cliente\n");

}

	/*
	orden = *(char*)recibir(socket_Kernel);
	printf("Orden:%c\n",orden);
	if(orden == 'C'){


	char * mensaje = recibir_string(socket_Kernel);
	printf("%s\n",mensaje);
	}
	*/

	return 0;
}
void leerConfiguracion(char* ruta) {
	configuracion_memoria = config_create(ruta);
	ipMemoria = config_get_string_value(configuracion_memoria, "IP_MEMORIA");
	puertoKernel= config_get_string_value(configuracion_memoria, "PUERTO_KERNEL");
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO_MEMORIA");
	ipKernel = config_get_string_value(configuracion_memoria,"IP_KERNEL");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nPUERTO KERNEL:%s\nIP KERNEL:%s\nPUERTO MEMORIA:%s\nIP MEMORIA:%s\n",puertoKernel,ipKernel,puertoMemoria,ipMemoria);
	printf("---------------------------------------------------\n");
}

