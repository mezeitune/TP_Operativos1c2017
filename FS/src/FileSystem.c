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
#include <commons/log.h>
#include <commons/bitarray.h>

char *ipFS;

t_bitarray * bit;

char *puertoFS;
char *puntoMontaje;
char *puerto_Kernel;
int tamanioBloques;
int cantidadBloques;
char* magicNumber;
int contadorConexiones=0;
pthread_t  thread_id;
t_config* configuracion_FS;

//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//


int socket_servidor;

int recibirConexion(int socket_servidor);
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

	bit = bitarray_create_with_mode(bitarray, cantidadBloques/8, LSB_FIRST);

	   FILE *fp;

	   fp = fopen( "../metadata/alumno.bin" , "w" );
	   fwrite(&bit , 1 , sizeof(t_bitarray) , fp );
	//int socket_kernel = crear_socket_cliente("127.0.0.1",puerto_Kernel);
	int socket_FS = crear_socket_servidor(ipFS,puertoFS);


	int socket_Kernel= recibirConexion(socket_FS);
	connection_handlerR(socket_Kernel);







	//log_info(loggerConPantalla, "File System Conectado");



	//TODO LO QUE ESTA COMENTADO ARRIBA TENDRIA Q IR DESCOMENTADO , PERO ESTOY USANDO ESTA
				//FUNCION MOMENTANEAMENTE , POR QUE EL KERNEL TODAVIA NO SE COMUNICA CON FS
				//ENTONCES LO HAGO PARA TESTEAR , NO BORRAR LAS OTRAS FUNCIONES QUE HAY


	return 0;
}


char nuevaOrdenDeAccion(int socketCliente)
{
	char* buffer=malloc(sizeof(char));
	char bufferRecibido;

	printf("\n--Esperando una orden del cliente-- \n");

	//recv(socketCliente,buffer,sizeof(char),0);
	bufferRecibido = *buffer;

	free(buffer);

    printf("El cliente ha enviado la orden: %c\n",bufferRecibido);
	printf("%c\n",bufferRecibido);
	return bufferRecibido;
}




void connection_handlerR(int socket_cliente)
{
	printf("fmksdmmsk");
    char orden;
    char* nombreArchivo;
    int tamanoArchivo;
    recv(socket_cliente,&orden,sizeof(char),0);
    FILE *fp;
    int resultadoDeEjecucion;
    while(orden != 'Q'){

    			//printf("\nIngresar orden:\n");
    			//scanf(" %c", &orden);

    	switch(orden){
		case 'V'://validar archivo   TERMINADO (FALTA QUE RECIBA EL ARCHIVO QUE SOLICITE DESDE KERNEL)
		    recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
		    recv(socket_cliente,&nombreArchivo,tamanoArchivo,0);
		    char* nombreArchivoRecibido=strcat("../metadata/", &nombreArchivo);
			if( access(nombreArchivoRecibido , F_OK ) != -1 ) {
			    // file exists
				printf("el archivo existe");
				send(socket_cliente,1,sizeof(int),0);
			} else {
			    // file doesn't exist
			   printf("Archivo inexistente");
				send(socket_cliente,2,sizeof(int),0);
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








 /*get:  read n bytes from position pos */
 char* obtenerBytesDeUnArchivo(FILE *fp, int offset, int size)
 {

	 	char aDevolver[size-offset];
		int caracterALeer;
		int paraDeLeer=size+offset;
		char name[2];
	    while((getc(fp)!=EOF))
	    {
	    	caracterALeer = fgetc(fp);
	        fseek(fp,offset,0);
	        char carALeerToChar=caracterALeer;
	        fgets(name,1,fp);
	        strcat(aDevolver, &carALeerToChar); /* copy name into the new var */
	        offset++ ;
	        if(offset==paraDeLeer) break;
	    }
	   fclose(fp);

	   return aDevolver;
 }







void leerConfiguracion(char* ruta){
	configuracion_FS = config_create(ruta);

	puertoFS= config_get_string_value(configuracion_FS,"PUERTO_FS");
	ipFS= config_get_string_value(configuracion_FS, "IP_FS");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");
	puerto_Kernel= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
}





void leerConfiguracionMetadata(char* ruta){
	configuracion_FS = config_create(ruta);

	tamanioBloques= config_get_string_value(configuracion_FS,"TAMANIO_BLOQUES");
	cantidadBloques= config_get_string_value(configuracion_FS, "CANTIDAD_BLOQUES");
	magicNumber = config_get_string_value(configuracion_FS,"MAGIC_NUMBER");


}


void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP FS:%s\nPUERTO FS:%s\nPUNTO MONTAJE:%s\n",ipFS,puertoFS,puntoMontaje);
		printf("\n \nTAMANIO BLOQUS:%s\nCANTIDAD BLQOUES:%s\nMAGIC NUMBER:%s\n",tamanioBloques,cantidadBloques,magicNumber);
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
	printf("sssssssssss");

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

