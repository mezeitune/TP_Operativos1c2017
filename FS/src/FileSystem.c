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
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conexiones.h"
#include "permisos.h"
#include "configuracionesLib.h"
#include "funcionesFS.h"
#include <commons/bitarray.h>

t_bitarray * bit;

//----------------------------//

int socket_servidor;

char nuevaOrdenDeAccion(int puertoCliente);
void leerConfiguracion(char* ruta);
void leerConfiguracionMetadata(char* ruta);
void imprimirConfiguraciones();
void connection_handlerR();
void printFilePermissions(char* archivo);
int archivoEnModoEscritura(char* archivo);
int archivoEnModoLectura(char* archivo);
char* obtenerBytesDeUnArchivo(FILE *fp, int offset, int size);

char *bitarray;

int main(void){

	//TODO:
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/FS/config_FileSys");
	leerConfiguracionMetadata("/home/utnso/workspace/tp-2017-1c-servomotor/FS/metadata/Metadata.bin");
	//leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/File\\System/config_FileSys");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logFS.txt");

	bit = bitarray_create_with_mode(bitarray, cantidadBloques, LSB_FIRST);


	int socket_FS = crear_socket_servidor(ipFS,puertoFS);


	int socket_Kernel= recibirConexion(socket_FS);
	connection_handlerR(socket_Kernel);



	return 0;
}



void connection_handlerR(int socket_cliente)
{
    char orden;

    recv(socket_cliente,&orden,sizeof(char),0);

    while(orden != 'Q'){

    	switch(orden){
		case 'V'://validar archivo
			validarArchivoFunction(socket_cliente);

			break;
		case 'C'://crear archivo
			crearArchivoFunction(socket_cliente);

			break;
		case 'B'://borrar archivo
			borrarArchivoFunction(socket_cliente);

			break;
		case 'O'://obtener datos
			obtenerDatosArchivoFunction(socket_cliente);


			break;
		case 'G'://guardar archivo
			guardarDatosArchivoFunction(socket_cliente);

			break;
		default:
			log_warning(loggerConPantalla,"\nError: Orden %c no definida\n",orden);
			break;
		}
	}
}








