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
#endif
