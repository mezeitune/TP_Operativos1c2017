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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/mman.h>



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


int main(void){
	//TODO:
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/FS/config_FileSys");
	leerConfiguracionMetadata("/home/utnso/workspace/tp-2017-1c-servomotor/FS/metadata/Metadata.bin");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logFS.txt");


	FILE *f;
	    f = fopen("../metadata/Bitmap.bin", "wr+");
	    int i;
	    for ( i=0;i<5192; i++) {
	       fputc(1, f);
	    }
	    fclose(f);


    int size;
    struct stat s;
    const char * file_name = "../metadata/Bitmap.bin";
    int fd = open ("../metadata/Bitmap.bin", O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;
	mmapDeBitmap = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);


//(tamanioBloquess*cantidadBloquess)/(8*tamanioBloquess)
	printf("\n\n\n %s \n\n\n\n\n",tamanioBloques);
	int tamanioBloquess=atoi(tamanioBloques);
	int cantidadBloquess=atoi(cantidadBloques);
	bitarray = bitarray_create_with_mode(mmapDeBitmap,(tamanioBloquess*cantidadBloquess)/(8*tamanioBloquess), LSB_FIRST);

	printf("%d\n\n\n",bitarray_get_max_bit(bitarray));

	int j;
	for(j=0;j<10;j++){
		if(bitarray_test_bit(bitarray, j)==1){
			printf("ocupado");
		}else{
			printf("liberado");
		}
	}


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








