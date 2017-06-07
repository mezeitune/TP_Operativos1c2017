#ifndef _CAPAFS_
#define _CAPAFS_
#include "conexiones.h"



//--------Capa FS--------//

char** tablaGlobalArchivos;
typedef struct FS{//Para poder guardar en la lista
	int pid;
	char** tablaArchivoPorProceso;
}t_tablaArchivoPorProceso;
t_list* listaTablasArchivosPorProceso;




int validarArchivoFS(char* ruta){
	char orden = 'V';
	char* archivoAVerificar=ruta;
	int tamano=sizeof(int)*strlen(archivoAVerificar);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,archivoAVerificar,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);


	return validado;
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

void abrirArchivoEnTablas(int socket_aceptado){
	int pid;
	int tamanoDireccion;
	int tamanoFlags;
	void* direccion = malloc(tamanoDireccion);
	void* flags = malloc(tamanoFlags);
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&tamanoDireccion,sizeof(int),0);
	recv(socket_aceptado,direccion,tamanoDireccion,0);
	recv(socket_aceptado,&tamanoFlags,sizeof(int),0);
	recv(socket_aceptado,flags,tamanoFlags,0);


	char* direccionAValidar=string_new();
	string_append(&direccionAValidar, direccion);


	int elArchivoExiste=validarArchivoFS(direccionAValidar);

	if(elArchivoExiste==1){

		//me fijo que exista en la tabla global de archivos
		/*int j,i;
		for(i = 0; i < nrows; ++i)
		{
		   for(j = 0; j<3 ; j++)
		   {
		      printf("%d\t",tablaGlobalArchivos[i][0]);
		   }
		printf("\n");
		}*/

		//sino existe , la agrego al ultimo coso

		//si existe le aumento el Open y lo abro en su tabla con su pid correspondiente y sus flags


		int validado=1;
		int descriptor=2;
		send(socket_aceptado,&descriptor,sizeof(int),0);
		send(socket_aceptado,&validado,sizeof(int),0);
	}else{
		int validado=0;
		send(socket_aceptado,&validado,sizeof(int),0);
		send(socket_aceptado,&validado,sizeof(int),0);
	}


}


void interfazHandlerParaFileSystem(char orden,int socket_aceptado){
		log_info(loggerConPantalla,"Iniciando Interfaz Handler Para File System\n");
		//int pid;
		char* mensajeRecibido;


		switch(orden){
				case 'A'://abrir archivo
					printf("abriendo un archivo en las tablas \n");
					abrirArchivoEnTablas(socket_aceptado);
					break;
				case 'V'://validar archivo
					printf("Validando que el archivo indicado exista \n");
					validarArchivoFS("alumno.bin");
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
