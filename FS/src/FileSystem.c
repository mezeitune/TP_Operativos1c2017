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
    int tamanoArchivo;
    FILE *fp;
    int resultadoDeEjecucion;
    int validado;

    recv(socket_cliente,&orden,sizeof(char),0);

    while(orden != 'Q'){

    			//printf("\nIngresar orden:\n");
    			//scanf(" %c", &orden);

    	switch(orden){
		case 'V'://validar archivo   TERMINADO (FALTA QUE RECIBA EL ARCHIVO QUE SOLICITE DESDE KERNEL)

		    recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
		    void* nombreArchivo = malloc(tamanoArchivo);
		    recv(socket_cliente,nombreArchivo,tamanoArchivo,0);

		    printf("Recibi el nombre del archivo\n ");


			char *nombreArchivoRecibido = string_new();
			string_append(&nombreArchivoRecibido, "../metadata/");
			string_append(&nombreArchivoRecibido, nombreArchivo);
		    printf("%s", nombreArchivoRecibido);
			if( access(nombreArchivoRecibido , F_OK ) != -1 ) {
			    // file exists
				printf("\n el archivo existe\n");

				validado=1;
				send(socket_cliente,&validado,sizeof(int),0);
			} else {
			    // file doesn't exist
			   printf("\n Archivo inexistente");

			   validado=0;
			   send(socket_cliente,&validado,sizeof(int),0);
			}

		    printf("\n ");//esto tiene que estar , no se por que
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
			if( access( "../metadata/nuevo.bin", F_OK ) != -1 ) {


			   fp = fopen("../metadata/nuevo.bin", "w");


			   if(remove("../metadata/nuevo.bin") == 0)
			   {
			      printf("File deleted successfully");
			   }
			   else
			   {
			      printf("Error: unable to delete the file");
			   }
			} else {
			    // file doesn't exist
				printf("Archivo inexistente");
			}

			   //falta marcar los bloques como libres dentro del bitmap
			break;
		case 'O'://obtener datos
			if( access( "../metadata/alumno.bin", F_OK ) != -1 ) {


				printFilePermissions("../metadata/alumno.bin");
				if((archivoEnModoLectura("../metadata/alumno.bin"))==1){
					//printf("\n dale sigamo");
					fp = fopen("../metadata/alumno.bin", "r");
					printf("\n %s",obtenerBytesDeUnArchivo(fp, 5, 9));


				}else{
					printf("\n El archivo no esta en modo lectura");
				}
			} else {
			    // file doesn't exist
				printf("Archivo inexistente");
			}

			break;
		case 'G'://guardar archivo
			if( access( "../metadata/alumno.bin", F_OK ) != -1 ) {

				printFilePermissions("../metadata/alumno.bin");
				if(archivoEnModoEscritura("../metadata/alumno.bin")==1){
					printf("\n dale sigamo");
				}else{
					printf("\n El archivo no esta en modo escritura");
				}

			} else {
			    // file doesn't exist
				printf("Archivo inexistente");
			}
			break;
		default:
			log_warning(loggerConPantalla,"\nError: Orden %c no definida\n",orden);
			break;
		}
	}
}








