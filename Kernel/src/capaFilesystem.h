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
}t_indiceTablaProceso;

typedef struct{
	int fd;
	char* flags;
	int globalFd;
	int puntero;
}t_entradaTablaProceso;

t_list* tablaArchivosGlobal;
t_list* listaTablasProcesos;

/*
 * Mutex a agregar:
 * mutexTablaArchivosGlobal;
 * mutexListaTablas
 * Habria que pensar si tengo que agregar un mutex cuando accedo a la tabla de un proceso en particular,
 * oooo bien, me conformo con el mutex de la lista de las tablas de todos los procesos
 * */


void interfaceHandlerFileSystem(int socket);

void abrirArchivo(int socket);
int validarArchivoFS(char* ruta);
int crearArchivo(int socket_aceptado, char* direccion );

void escribirArchivo(int socket);
void borrarArchivo(int socket);

/*Tabla global*/
void aumentarOpenEnTablaGlobal(char* direccion);
int agregarEntradaEnTablaGlobal(char* direccion,int tamanioDireccion);
int verificarEntradaEnTablaGlobal(char* direccion);
void verificarAperturaFdEnTablaGlobal(int indiceGlobalFd);
int buscarIndiceEnTablaGlobal(char* direccion);
char* buscarDireccionEnTablaGlobal(int indice);

/*Tabla del proceso*/
int actualizarTablaDelProceso(int pid,char* flags, int indiceEnTablaGlobal);
void inicializarTablaProceso(int pid);
int borrarEntradaTablaProceso(int pid,int fileDescriptor);

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

		int archivoExistente=validarArchivo(direccion);
		
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

void borrarArchivo(int socket){
	int pid;
		int fileDescriptor;
		int resultadoEjecucion ;
		recv(socket,&pid,sizeof(int),0);
		recv(socket,&fileDescriptor,sizeof(int),0);
		log_info(loggerConPantalla,"Borrando un archivo--->PID:%d--->FD:%d",pid,fileDescriptor);

		_Bool verificaPid(t_indiceTablaProceso* entrada){
						return entrada->pid == pid;
					}
		_Bool verificaFd(t_entradaTablaProceso* entrada){
					return entrada->fd == fileDescriptor;
				}

		//verificar que la tabla de ese pid exista
		int tablaProcesoExiste;
		if(list_any_satisfy(listaTablasProcesos,(void*)verificaPid)) tablaProcesoExiste = 1;
		else tablaProcesoExiste = 0;

		if(!tablaProcesoExiste){
			excepcionArchivoInexistente(socket,pid);
			return;
		}else{
			int encontroFd;
			t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
			if(list_any_satisfy(entradaTablaProceso->tablaProceso,(void*)verificaFd)) encontroFd = 1;
			else encontroFd = 0;
			list_add(listaTablasProcesos,entradaTablaProceso);

			if(!encontroFd){
				excepcionArchivoInexistente(socket,pid);
			}else{
				//hacer los sends para que el FS borre ese archivo y deje los bloques libres
				char comandoBorrarArchivo='B';
				send(socketFyleSys,comandoBorrarArchivo,sizeof(char),0);
				//send(socketFyleSys,&tamanioArchivo,sizeof(int),0); TODO: Encontrar el size del file
				//send(socketFyleSys,nombreArchivo,strlen(nombreArchivo)*sizeof(char),0); TODO: Encontrar el nombre del archivo
				recv(socketFyleSys,&resultadoEjecucion,sizeof(char),0);
					if(resultadoEjecucion < 0) {
						excepcionFileSystem(socket,pid);
						return;
					}
				int indiceGlobalFd=borrarEntradaTablaProceso(pid,fileDescriptor);
				verificarAperturaFdEnTablaGlobal(indiceGlobalFd);
			}

		}
		send(socket,&resultadoEjecucion,sizeof(int),0);
		log_info(loggerConPantalla,"Archivo borrado--->PID:%d",pid);
}

void escribirArchivo(int socket){
	int pid;
	int fileDescriptor;
	int resultadoEjecucion;
	int size;

		recv(socket,&pid,sizeof(int),0);
		printf("Pid:%d\n",pid);

		recv(socket,&fileDescriptor,sizeof(int),0);
		printf("FD:%d\n",fileDescriptor);

		recv(socket,&size,sizeof(int),0);
		printf("Size:%d\n",size);
		char* informacion = malloc(size);

	    recv(socket,informacion,size,0);
	    strcpy(informacion + size, "\0");
		printf("Data:%s\n",informacion);
		log_info(loggerConPantalla,"Guardando datos del archivo indicado--->PID:%d--->Datos:%s",pid,informacion);


		_Bool verificaPid(t_indiceTablaProceso* entrada){
				return entrada->pid == pid;
			}

		_Bool verificaFd(t_entradaTablaProceso* entrada){
			return entrada->fd == fileDescriptor;
		}

		//verificar que la tabla de ese pid exista
		int tablaProcesoExiste;
		if(list_any_satisfy(listaTablasProcesos,(void*)verificaPid)) tablaProcesoExiste = 1;
		else tablaProcesoExiste = 0;

		int encontroFd;

		if(!tablaProcesoExiste){
			excepcionArchivoInexistente(socket,pid);
			free(informacion);
			return;
		}

		if(tablaProcesoExiste){

			t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
			if(list_any_satisfy(entradaTablaProceso->tablaProceso,(void*)verificaFd)) encontroFd = 1;
			else encontroFd = 0;

			if(!encontroFd){
				excepcionArchivoInexistente(socket,pid);
			}

			if(encontroFd){
				t_entradaTablaProceso* entrada = list_remove_by_condition(entradaTablaProceso->tablaProceso,(void*)verificaFd);

				int tiene_permisoEscritura=0;
				const char *permiso_escritura = "w";
				if(string_contains(entrada->flags, permiso_escritura)){
					tiene_permisoEscritura=1;
				}

				if(!tiene_permisoEscritura){
					excepcionPermisosEscritura(socket,pid);
					free(informacion);
					return;
				}
				char* direccion = buscarDireccionEnTablaGlobal(entrada->globalFd);

					char** array_dir=string_n_split(direccion, 12, "/"); /*TODO WTF is this*/
					  /* int d=0;
					   while(!(array_dir[d] == NULL)){
					      d++;
					   }
					   */
					char* nombreArchivo=array_dir[0];
					int tamanoNombre=sizeof(char)*strlen(nombreArchivo);

					send(socketFyleSys,&tamanoNombre,sizeof(int),0);
					send(socketFyleSys,nombreArchivo,tamanoNombre,0);
					send(socketFyleSys,&entrada->puntero,sizeof(int),0);
					send(socketFyleSys,&size,sizeof(int),0);
					send(socketFyleSys,informacion,size,0);

					recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);
			}

		}
		send(socket,&resultadoEjecucion,sizeof(int),0);
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


int actualizarTablaDelProceso(int pid,char* flags,int indiceEnTablaGlobal){
	log_info(loggerConPantalla,"Agregando entrada a tabla por proceso");
	int tablaProcesoExiste;

	_Bool verificaPid(t_indiceTablaProceso* entrada){
		return entrada->pid == pid;
	}

	if(list_any_satisfy(listaTablasProcesos,(void*)verificaPid)) tablaProcesoExiste = 1;
	else tablaProcesoExiste = 0;


	 if(!tablaProcesoExiste){
		 printf("La tabla no existe\n");
		 t_indiceTablaProceso* entradaNuevaTabla = malloc(sizeof(t_indiceTablaProceso));
		 entradaNuevaTabla->pid = pid;
		 entradaNuevaTabla->tablaProceso = list_create();

		 t_entradaTablaProceso* entrada=malloc(sizeof(t_entradaTablaProceso));
		 entrada->fd = 3;
		 entrada->flags = flags;
		 entrada->globalFd = indiceEnTablaGlobal;
		 entrada->puntero=0;

		 list_add(entradaNuevaTabla->tablaProceso,entrada);

		 list_add(listaTablasProcesos,entradaNuevaTabla);
			return entrada->fd;
		}
	 else{
		 printf("La tabla ya existe\n");
		 t_indiceTablaProceso* entradaTablaExistente = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
		 t_entradaTablaProceso* entrada = malloc(sizeof(t_entradaTablaProceso));
		 entrada->fd = entradaTablaExistente->tablaProceso->elements_count + 3;
		 entrada->flags = flags;
		 entrada->globalFd = indiceEnTablaGlobal;

		 list_add(entradaTablaExistente->tablaProceso,entrada);
		 list_add(listaTablasProcesos,entradaTablaExistente);

		 return entrada->fd;
	 }
}

int borrarEntradaTablaProceso(int pid,int fd){
	int globalFd;
	_Bool verificaPid(t_indiceTablaProceso* entrada){
							return entrada->pid == pid;
						}
	_Bool verificaFd(t_entradaTablaProceso* entrada){
			return entrada->fd == fd;
		}
	t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
	t_entradaTablaProceso* entrada = list_remove_by_condition(entradaTablaProceso->tablaProceso,(void*)verificaFd);
	globalFd = entrada->globalFd;
	free(entrada);
	list_add(listaTablasProcesos,entradaTablaProceso);
	return globalFd;
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

void verificarAperturaFdEnTablaGlobal(int indiceGlobalFd){
	log_info(loggerConPantalla,"Verificando apertura en tabla global");
	t_entradaTablaGlobal* entrada = list_get(tablaArchivosGlobal,indiceGlobalFd);
	entrada->open --;

	if(entrada->open==0){
		list_remove(tablaArchivosGlobal,indiceGlobalFd);
		free(entrada);
	}
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

char* buscarDireccionEnTablaGlobal(int indice){
	t_entradaTablaGlobal* entrada = list_get(tablaArchivosGlobal,indice);

	return entrada->path;
}

void inicializarTablaProceso(int pid){
	 printf("La tabla no existe\n");
	 t_indiceTablaProceso* entradaNuevaTabla = malloc(sizeof(t_indiceTablaProceso));
	 entradaNuevaTabla->pid = pid;
	 entradaNuevaTabla->tablaProceso = list_create();

	 t_entradaTablaProceso* entrada=malloc(sizeof(t_entradaTablaProceso));
	 entrada->fd = 0;
	 entrada->flags = "rwc";
	 entrada->globalFd = 0;
	 entrada->puntero=0;

	 list_add(entradaNuevaTabla->tablaProceso,entrada);

	 list_add(listaTablasProcesos,entradaNuevaTabla);
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
					case 'A':	abrirArchivo(socket);
								break;
					case 'B':	borrarArchivo(socket); /*TODO: Falta sends y recvs*/
								break;
					case 'O'://obtenerArchivoFS(socket); /*TODO: Falta desarrollar: leerArchivo(socket)*/
						break;
					case 'G': 	escribirArchivo(socket); /*TODO: Falta testear*/
								break;
					case 'P':	//cerrarArchivoFS(socket); /*TODO: Falta desarrollar: cerrarArchivo(socket)*/
								break;
					case 'M':	//moverCursorArchivoFS(socket); /*TODO: Falta desarrollar: moverCursosArchivo(socket)*/
								break;
					default:
					log_error(loggerConPantalla ,"Orden no reconocida: %c",orden);
					break;
				}
		log_info(loggerConPantalla,"Finalizando atencion de Interfaz Handler de File System");
}



#endif /* CAPAFILESYSTEM_H_ */
