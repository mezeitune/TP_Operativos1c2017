/*
 * capaFilesystem.h
 *
 *  Created on: 28/6/2017
 *      Author: utnso
 */

#ifndef CAPAFILESYSTEM_H_
#define CAPAFILESYSTEM_H_
#include "sockets.h"
#include <commons/collections/list.h>

typedef struct{
	char* path;
	int open;
}t_entradaTablaGlobal;

typedef struct{
	int pid;
	t_list* tablaProceso;
}t_entradaListaTablas;

typedef struct{
	int fd;
	char* flags;
	int globalFd;
}t_entradaTablaProceso;

t_list* tablaArchivosGlobal;
t_list* listaTablas;

void interfaceHandlerFileSystem(int socket);



void abrirArchivo(int socket);
int validarArchivoFS(char* ruta);
int crearArchivo(int socket_aceptado, char* direccion );

/*Tabla global*/
void aumentarOpenEnTablaGlobal(char* direccion);
int agregarEntradaEnTablaGlobal(char* direccion,int tamanioDireccion);
int verificarEntradaEnTablaGlobal(char* direccion);
int buscarIndiceEnTablaGlobal(char* direccion);

/*Tabla del proceso*/
int actualizarTablaDelProceso(int pid,char* flags, int indiceEnTablaGlobal);


void abrirArchivo(int socket){
	log_info(loggerConPantalla,"Abriendo un archivo");
		int pid;
		int tamanoDireccion;
		int tamanoFlags;
		char* direccion;
		char* flags;

		int fileDescriptor;
		int indiceEnTablaGlobal;
		int resultadoEjecucion;

		recv(socket,&pid,sizeof(int),0);
		printf("PID:%d\n",pid);

		recv(socket,&tamanoDireccion,sizeof(int),0);
		printf("Tamano direccion%d\n",tamanoDireccion);
		direccion = malloc(tamanoDireccion);

		recv(socket,direccion,tamanoDireccion,0);
		strcpy(direccion + tamanoDireccion, "\0");
		printf("Direccion %s\n",direccion);

		recv(socket,&tamanoFlags,sizeof(int),0);
		printf("Tamano flags%d\n",tamanoFlags);
		flags= malloc(tamanoFlags);

		recv(socket,flags,tamanoFlags,0);
		strcpy(flags + tamanoFlags, "\0");
		printf("Flags %s\n",flags);

		int archivoExistente=validarArchivoFS(direccion);
		
		int tienePermisoCreacion=0;
		char* permiso_creacion = "c";

		if(string_contains(flags,permiso_creacion)){
			tienePermisoCreacion=1;
		}


		if(!archivoExistente && !tienePermisoCreacion){
					excepcionPermisosCrear(socket,pid);
					free(direccion);
					free(flags);
					return;
		}


		if(archivoExistente){
			int entradaGlobalExistente=verificarEntradaEnTablaGlobal(direccion);

			if(!entradaGlobalExistente){
				indiceEnTablaGlobal = agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);
				printf("No estaba en la tabla global\n");
				printf("Indice devuelto:%d\n",indiceEnTablaGlobal);
				}
			else{
				indiceEnTablaGlobal = buscarIndiceEnTablaGlobal(direccion);
				printf("Ya estaba en la tabla global\n");
			}
		}

		if(!archivoExistente && tienePermisoCreacion){

			resultadoEjecucion=crearArchivo(socket,direccion);
			if(resultadoEjecucion < 0){
				excepcionFileSystem(socket,pid);
				free(direccion);
				free(flags);
				return;
			}
		indiceEnTablaGlobal=agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);
		}

		aumentarOpenEnTablaGlobal(direccion);
		fileDescriptor = actualizarTablaDelProceso(pid,flags,indiceEnTablaGlobal);
		resultadoEjecucion = 1;
		 send(socket,&resultadoEjecucion,sizeof(int),0);
		 printf("resultado : %d\n",resultadoEjecucion);
		 send(socket,&fileDescriptor,sizeof(int),0);
		 printf("File descriptor:%d\n",fileDescriptor);
		 printf("Mande todo\n");
		log_info(loggerConPantalla,"Finalizo la apertura del archivo");
}


int actualizarTablaDelProceso(int pid,char* flags,int indiceEnTablaGlobal){
	log_info(loggerConPantalla,"Agregando entrada a tabla por proceso");
	int tablaProcesoExiste;

	_Bool verificaPid(t_entradaListaTablas* entrada){
		return entrada->pid == pid;
	}

	if(list_any_satisfy(listaTablas,(void*)verificaPid)) tablaProcesoExiste = 1;
	else tablaProcesoExiste = 0;

	 if(!tablaProcesoExiste){
		 printf("La tabla no existe\n");
		 t_entradaListaTablas* entradaNuevaTabla = malloc(sizeof(t_tablaArchivoPorProceso));
		 entradaNuevaTabla->pid = pid;
		 entradaNuevaTabla->tablaProceso = list_create();

		 t_entradaTablaProceso* entrada=malloc(sizeof(t_entradaTablaProceso));
		 entrada->fd = 3;
		 entrada->flags = flags;
		 entrada->globalFd = indiceEnTablaGlobal;

		 list_add(entradaNuevaTabla->tablaProceso,entrada);

		 list_add(listaTablas,entradaNuevaTabla);
			return entrada->fd;
		}
	 else{
		 printf("La tabla ya existe\n");
		 t_entradaListaTablas* entradaTablaExistente = list_remove_by_condition(listaTablas,(void*)verificaPid);
		 t_entradaTablaProceso* entrada = malloc(sizeof(t_entradaTablaProceso));
		 entrada->fd = entradaTablaExistente->tablaProceso->elements_count + 3;
		 entrada->flags = flags;
		 entrada->globalFd = indiceEnTablaGlobal;

		 list_add(entradaTablaExistente->tablaProceso,entrada);
		 list_add(listaTablas,entradaTablaExistente);

		 return entrada->fd;
	 }
}





int crearArchivo(int socket_aceptado, char* direccion ){
	log_info(loggerConPantalla,"Creando archivo en FileSystem---> %s",direccion);

	int validado;

	char** array_dir=string_n_split(direccion, 12, "/"); /*TODO: HArcodeado*/
	/*   int d=0;
	   while(!(array_dir[d] == NULL)){
	      d++;
	   }
	   */
	char* nombreArchivo=array_dir[0];
	int tamanoNombre=sizeof(char)*strlen(nombreArchivo);
	char ordenCrearArchivo = 'C';
	send(socketFyleSys,&ordenCrearArchivo,sizeof(char),0);
	send(socketFyleSys,&tamanoNombre,sizeof(int),0);
	send(socketFyleSys,nombreArchivo,tamanoNombre,0);

	recv(socketFyleSys,&validado,sizeof(int),0);

	if(validado<0) log_error(loggerConPantalla,"Error al crear archivo por excepecion de FileSystem--->",direccion);
	else log_info(loggerConPantalla,"Archivo creado en FileSystem---> %s",direccion);
	return validado;

}


int agregarEntradaEnTablaGlobal(char* direccion,int tamanioDireccion){
	log_info(loggerConPantalla,"Agregando entrada en tabla global--->Direccion:%s",direccion);
	t_entradaTablaGlobal* entrada = malloc(sizeof(t_entradaTablaGlobal));
	entrada->open = 0;
	entrada->path = malloc(tamanioDireccion);
	entrada->path=direccion;

	list_add(tablaArchivosGlobal,entrada);

	return tablaArchivosGlobal->elements_count - 1 ;
}

int verificarEntradaEnTablaGlobal(char* direccion){ /*TODO: Mutex tablaGlobal*/
	log_info(loggerConPantalla,"Verificando que exista entrada en tabla global--->Direccion:%s",direccion);

	_Bool verificaDireccion(t_entradaTablaGlobal* entrada){
		return !strcmp(entrada->path,direccion);
	}

	if(list_is_empty(tablaArchivosGlobal)) return 0;
	printf("La tabla global no esta vacia\n");

	if(list_any_satisfy(tablaArchivosGlobal,(void*)verificaDireccion)) return 1;
	return 0;

}

void aumentarOpenEnTablaGlobal(char* direccion){/*TODO: Mutex tablaGlobal*/
	log_info(loggerConPantalla,"Aumentando open en tabla global--->Direccion:%s",direccion);
	_Bool verificaDireccion(t_entradaTablaGlobal* entrada){
			if(!strcmp(entrada->path,direccion)) return 1;
			return 0;
		}

	t_entradaTablaGlobal* entrada=list_remove_by_condition(tablaArchivosGlobal,(void*)verificaDireccion);
	entrada->open ++;
	list_add(tablaArchivosGlobal,entrada);
}

int buscarIndiceEnTablaGlobal(char* direccion){
	int i;
	int indice=0;
	t_entradaTablaGlobal* entrada;

	for(i=0;i<tablaArchivosGlobal->elements_count;i++){
		entrada = list_get(tablaArchivosGlobal,i);
		if(!strcmp(entrada->path,direccion)) indice = i;
	}

	return indice;
}

int validarArchivo(char* ruta){
	log_info(loggerConPantalla,"Validando que el archivo exista--->Ruta:%s",ruta);
	char ordenValidarArchivo = 'V';
	int tamano=sizeof(int)*strlen(ruta);
	int validado;
	send(socketFyleSys,&ordenValidarArchivo,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,ruta,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);

	log_info(loggerConPantalla,"Archivo validado--->Ruta:%s",ruta);
	return validado;
}
void interfaceHandlerFileSystem(int socket){
			log_info(loggerConPantalla,"Iniciando Interfaz Handler Para File System");
			char orden;
			recv(socket,&orden,sizeof(char),0);
			switch(orden){
					case 'A'://abrir archivo
						abrirArchivo(socket);
						break;
					case 'V'://validar archivo
						validarArchivo("alumno.bin");
						break;
					case 'B'://borrar archivo
						borrarArchivoFS(socket);
						break;
					case 'O'://obtener datos
						obtenerArchivoFS(socket);
						break;
					case 'G'://guardar archivo
						guardarArchivoFS(socket);
						break;
					case 'P'://guardar archivo
						cerrarArchivoFS(socket);
						break;
					case 'M'://guardar archivo
						moverCursorArchivoFS(socket);
						break;
				default:
					log_error(loggerConPantalla ,"Orden no reconocida: %c",orden);
					break;
				}
		log_info(loggerConPantalla,"Finalizando atencion de Interfaz Handler de File System");
}



#endif /* CAPAFILESYSTEM_H_ */
