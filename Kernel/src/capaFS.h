#ifndef _CAPAFS_
#define _CAPAFS_
#include "conexiones.h"



//--------Capa FS--------//

char** tablaGlobalArchivos;
typedef struct FS{//Para poder guardar en la lista
	char** tablaArchivoPorProceso;
}t_tablaArchivoPorProceso;
t_list* listaTablasArchivosPorProceso;

void validarArchivoFS(){
	char orden = 'V';
	char* archivoAVerificar="alumno.bin";
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion fue %d \n",validado);
}

void crearArchivoFS(){
	char orden = 'V';
	char* archivoAVerificar="alumno.bin";
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion fue %d \n",validado);
}

void borrarArchivoFS(){
	char orden = 'V';
	char* archivoAVerificar="alumno.bin";
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion fue %d \n",validado);
}

void obtenerArchivoFS(){
	char orden = 'V';
	char* archivoAVerificar="alumno.bin";
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion fue %d \n",validado);
}

void guardarArchivoFS(){
	char orden = 'V';
	char* archivoAVerificar="alumno.bin";
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);
	printf("La validacion fue %d \n",validado);
}


void interfazHandlerParaFileSystem(char orden){
		log_info(loggerConPantalla,"Iniciando Interfaz Handler Para File System\n");
		//int pid;
		char* mensajeRecibido;


		switch(orden){
				case 'V'://validar archivo
					printf("Validando que el archivo indicado exista \n");
					validarArchivoFS();
					break;
				case 'C'://crear archivo
					printf("Creando el archivo indacdo \n");
					crearArchivoFS();
					break;
				case 'B'://borrar archivo
					printf("Borrando el archivo indacado \n");
					borrarArchivoFS();
					break;
				case 'O'://obtener datos
					printf("Obteniendo datos del archivo indicado \n");
					obtenerArchivoFS();
					break;
				case 'G'://guardar archivo
					printf("Guardando datos del archivo indicado \n");
					guardarArchivoFS();
					break;
			default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla ,"\nOrden no reconocida\n");
				break;
			}
			orden = '\0';
			log_info(loggerConPantalla,"Finalizando atencion de Interfaz Handler de File System\n");
			return;

}
#endif
