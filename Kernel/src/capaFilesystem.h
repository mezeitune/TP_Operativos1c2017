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
#include "sincronizacion.h"

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

typedef struct{
	int socket;
	int pid;
	int tamanoDireccion;
	int tamanoFlags;
	char* direccion;
	char* flags;
}t_fsAbrir;

typedef struct{
	int socket;
	int pid;
	int fd;
	int size;
	char* informacion;
}t_fsEscribir;

typedef struct{
	int socket;
	int pid;
	int fd;
	int size;
}t_fsLeer;


/*
 * Mutex a agregar:
 * mutexTablaArchivosGlobal;
 * mutexListaTablas
 * Habria que pensar si tengo que agregar un mutex cuando accedo a la tabla de un proceso en particular,
 * oooo bien, me conformo con el mutex de la lista de las tablas de todos los procesos
 * */


void interfaceHandlerFileSystem(int socket);
void interfaceAbrirArchivo(int socket);
void interfaceEscribirArchivo(int socket);
void interfaceLeerArchivo(int socket);

/*Interface*/
void abrirArchivo(t_fsAbrir* data);
void moverCursorArchivo(int socket);
void escribirArchivo(t_fsEscribir* data);
void leerArchivo(t_fsLeer* data);
void borrarArchivo(int socket);
void cerrarArchivo(int socket);

int validarArchivo(char* ruta);
int crearArchivo(int socket_aceptado, char* direccion );

/*Tabla global*/
void aumentarOpenEnTablaGlobal(char* direccion);
int agregarEntradaEnTablaGlobal(char* direccion,int tamanioDireccion);
int verificarEntradaEnTablaGlobal(char* direccion);
void disminuirOpenYVerificarExistenciaEntradaGlobal(int indiceTablaGlobal);
int buscarIndiceEnTablaGlobal(char* direccion);
char* buscarDireccionEnTablaGlobal(int indice);
int verificarArchivoAbiertoEnTablaGlobal(int indiceTablaGlobal);
void borrarEntradaEnTablaGlobal(int indiciTablaGlobal);

/*Tabla del proceso*/
int actualizarTablaDelProceso(int pid,char* flags, int indiceEnTablaGlobal);
int verificarFileDescriptorAbierto(int pid,int fileDescriptor);
void inicializarTablaProceso(int pid);
int borrarEntradaTablaProceso(int pid,int fileDescriptor);
void actualizarIndicesGlobalesEnTablasProcesos(int indiceTablaGlobal);
int buscarIndiceGlobalEnTablaProceso(int pid,int fileDescriptor);

void abrirArchivo(t_fsAbrir* data){

		int socket = data->socket;
		int pid=data->pid;
		int tamanoDireccion=data->tamanoDireccion;
		char* direccion=data->direccion;
		char* flags=data->flags;

		int fileDescriptor;
		int indiceEnTablaGlobal;
		int resultadoEjecucion=1;

		log_info(loggerConPantalla,"Abriendo un archivo--->PID:%d--->Direccion:%s--->Permisos:%s",pid,direccion,flags);

		pthread_mutex_lock(&mutexFS);
		int archivoExistente=validarArchivo(direccion);
		pthread_mutex_unlock(&mutexFS);
		
		int tienePermisoCreacion=0;
		char* permiso_creacion = "c";
		if(string_contains(flags,permiso_creacion)){
			tienePermisoCreacion=1;
		}


		if(!archivoExistente && !tienePermisoCreacion){//El archivo no eexiste en FS y no tiene permisos para crear entonces no hace nada
					excepcionPermisosCrear(socket,pid);
					free(direccion);
					free(flags);
					return;
		}


		if(archivoExistente){

			pthread_mutex_lock(&mutexTablaGlobal);
			int entradaGlobalExistente=verificarEntradaEnTablaGlobal(direccion);
			pthread_mutex_unlock(&mutexTablaGlobal);

			if(!entradaGlobalExistente){
				pthread_mutex_lock(&mutexTablaGlobal);
				indiceEnTablaGlobal = agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);//almacenar el Global FD
				pthread_mutex_unlock(&mutexTablaGlobal);
				}
			else{
				pthread_mutex_lock(&mutexTablaGlobal);
				indiceEnTablaGlobal = buscarIndiceEnTablaGlobal(direccion);
				pthread_mutex_unlock(&mutexTablaGlobal);
			}
		}

		if(!archivoExistente && tienePermisoCreacion){
			pthread_mutex_lock(&mutexFS);
			resultadoEjecucion=crearArchivo(socket,direccion);
			pthread_mutex_unlock(&mutexFS);
				if(resultadoEjecucion < 0){
					excepcionFileSystem(socket,pid);
					free(direccion);
					free(flags);
					return;
				}
			pthread_mutex_lock(&mutexTablaGlobal);
			indiceEnTablaGlobal=agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);
			pthread_mutex_unlock(&mutexTablaGlobal);
		}
		pthread_mutex_lock(&mutexTablaGlobal);
		aumentarOpenEnTablaGlobal(direccion);
		pthread_mutex_unlock(&mutexTablaGlobal);

		pthread_mutex_lock(&mutexListaTablaArchivos);
		fileDescriptor = actualizarTablaDelProceso(pid,flags,indiceEnTablaGlobal);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		 pthread_mutex_lock(&mutexFS);
		 send(socket,&resultadoEjecucion,sizeof(int),0);
		 send(socket,&fileDescriptor,sizeof(int),0);
		 pthread_mutex_unlock(&mutexFS);

		log_info(loggerConPantalla,"Finalizo la apertura del archivo--->PID:%d--->FD:%d",pid,fileDescriptor);
		free(data);
}

void borrarArchivo(int socket){
		int pid;
		int fileDescriptor;
		int resultadoEjecucion ;
		recv(socket,&pid,sizeof(int),0);
		recv(socket,&fileDescriptor,sizeof(int),0);
		log_info(loggerConPantalla,"Borrando un archivo--->PID:%d--->FD:%d",pid,fileDescriptor);

		_Bool verificaFd(t_entradaTablaProceso* entrada){
					return entrada->fd == fileDescriptor;
				}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		int fileDescriptorAbierto = verificarFileDescriptorAbierto(pid,fileDescriptor);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		if(!fileDescriptorAbierto){ /*Si ese archivo no lo tiene abierto no lo puede borrar*/
			excepcionFileDescriptorNoAbierto(socket,pid);
			return;
		}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		int indiceTablaGlobal = buscarIndiceGlobalEnTablaProceso(pid,fileDescriptor);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		pthread_mutex_lock(&mutexTablaGlobal);
		if(verificarArchivoAbiertoEnTablaGlobal(indiceTablaGlobal)<0){ //No lo puede borrar, y tampoco lo cierra.
			pthread_mutex_unlock(&mutexTablaGlobal);
			excepcionNoPudoBorrarArchivo(socket,pid);
			return;
		}
		pthread_mutex_unlock(&mutexTablaGlobal);

		pthread_mutex_lock(&mutexListaTablaArchivos);
		borrarEntradaTablaProceso(pid,fileDescriptor);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		pthread_mutex_lock(&mutexTablaGlobal);
		borrarEntradaEnTablaGlobal(indiceTablaGlobal); //No se disminuye el open,se borra la entrada, porque solo se puede borrar si es el unico proceso que lo tiene abierto.
		pthread_mutex_lock(&mutexTablaGlobal);

		pthread_mutex_lock(&mutexListaTablaArchivos);
		actualizarIndicesGlobalesEnTablasProcesos(indiceTablaGlobal);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		//hacer los sends para que el FS borre ese archivo y deje los bloques libres
		char comandoBorrarArchivo='B';
		char* direccion = buscarDireccionEnTablaGlobal(indiceTablaGlobal);
		char** array_dir=string_n_split(direccion, 12, "/");
		char* nombreArchivo=array_dir[0];
		int tamanoNombre=sizeof(char)*strlen(nombreArchivo);

		pthread_mutex_lock(&mutexFS);
		send(socketFyleSys,&comandoBorrarArchivo,sizeof(char),0);
		send(socketFyleSys,&tamanoNombre,sizeof(int),0);
		send(socketFyleSys,nombreArchivo,tamanoNombre,0);
		recv(socketFyleSys,&resultadoEjecucion,sizeof(char),0);
		pthread_mutex_unlock(&mutexFS);


			if(resultadoEjecucion < 0) {
				excepcionFileSystem(socket,pid);
				return;
			}


	send(socket,&resultadoEjecucion,sizeof(int),0);
	log_info(loggerConPantalla,"Archivo borrado--->PID:%d",pid);
}

void cerrarArchivo(int socket){
	int pid;
	int fileDescriptor;
	int resultadoEjecucion ;
	recv(socket,&pid,sizeof(int),0);
	recv(socket,&fileDescriptor,sizeof(int),0);
	log_info(loggerConPantalla,"Cerrando el archivo--->PID:%d--->FD:%d",pid,fileDescriptor);

	_Bool verificaPid(t_indiceTablaProceso* entrada){
						return entrada->pid == pid;
					}
	_Bool verificaFd(t_entradaTablaProceso* entrada){
					return entrada->fd == fileDescriptor;
				}


			int encontroFd;
			t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
			if(list_any_satisfy(entradaTablaProceso->tablaProceso,(void*)verificaFd)) encontroFd = 1;
			else encontroFd = 0;
			list_add(listaTablasProcesos,entradaTablaProceso);

			if(!encontroFd){
				excepcionFileDescriptorNoAbierto(socket,pid);
				return;
			}else{

				int indiceTablaGlobal=borrarEntradaTablaProceso(pid,fileDescriptor);
				disminuirOpenYVerificarExistenciaEntradaGlobal(indiceTablaGlobal);
			}
		resultadoEjecucion=1;
		send(socket,&resultadoEjecucion,sizeof(int),0);

}

void escribirArchivo(t_fsEscribir* data){
	char comandoGuardarDatos = 'G';
	int socket = data->socket;
	int pid=data->pid;
	int fileDescriptor=data->fd;
	int size = data->size;
	char* informacion = malloc(data->size);
	informacion = data->informacion;

	int resultadoEjecucion;

	log_info(loggerConPantalla,"Guardando datos del archivo indicado--->PID:%d--->Datos:%s",pid,informacion);


		_Bool verificaPid(t_indiceTablaProceso* entrada){
				return entrada->pid == pid;
			}

		_Bool verificaFd(t_entradaTablaProceso* entrada){
			return entrada->fd == fileDescriptor;
		}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		int fileDescriptorAbierto = verificarFileDescriptorAbierto(pid,fileDescriptor);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		if(!fileDescriptorAbierto){
			excepcionFileDescriptorNoAbierto(socket,pid);
			return;
		}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
		t_entradaTablaProceso* entrada = list_remove_by_condition(entradaTablaProceso->tablaProceso,(void*)verificaFd);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		int cursor = entrada->puntero;

		int tiene_permisoEscritura=0;
		char *permiso_escritura = "w";
		if(string_contains(entrada->flags, permiso_escritura)){
			tiene_permisoEscritura=1;
		}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		list_add(entradaTablaProceso->tablaProceso,entrada);
		list_add(listaTablasProcesos,entradaTablaProceso);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		if(!tiene_permisoEscritura){
			excepcionPermisosEscritura(socket,pid);
			free(informacion);
			return;
		}

			pthread_mutex_lock(&mutexTablaGlobal);
			char* direccion = buscarDireccionEnTablaGlobal(entrada->globalFd);
			pthread_mutex_unlock(&mutexTablaGlobal);


			char** array_dir=string_n_split(direccion, 12, "/");
			char* nombreArchivo=array_dir[0];
			int tamanoNombre=sizeof(char)*strlen(nombreArchivo);

			printf("Tamano del nombre :%d\n",tamanoNombre);
			printf("Nombre del archivo: %s\n",nombreArchivo);
			printf("Puntero:%d\n",cursor);
			printf("Tamano a escribir :%d\n",size);
			printf("Informacion a escribir:%s\n",informacion);


			pthread_mutex_unlock(&mutexFS);
			send(socketFyleSys,&comandoGuardarDatos,sizeof(char),0);
			send(socketFyleSys,&tamanoNombre,sizeof(int),0);
			send(socketFyleSys,nombreArchivo,tamanoNombre,0);
			send(socketFyleSys,&cursor,sizeof(int),0);
			send(socketFyleSys,&size,sizeof(int),0);
			send(socketFyleSys,informacion,size,0);

			recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);
			pthread_mutex_unlock(&mutexFS);

		printf("Resultado de ejecucion :%d\n",resultadoEjecucion);
		if(resultadoEjecucion < 0){
			excepcionFileSystem(socket,pid); /*TODO Esta rompiendo. Expropia, pero despues le llegan datos de mas*/
			free(informacion);
			return;
		}
		send(socket,&resultadoEjecucion,sizeof(int),0);
		free(informacion);
		free(data);
}

void leerArchivo(t_fsLeer* data){
	int socket = data->socket;
	int pid=data->pid;
	int fileDescriptor=data->fd;
	int tamanioALeer=data->size;
	char* informacion;

	char comandoLeer = 'O';
	int resultadoEjecucion;

	log_info(loggerConPantalla,"Leyendo de archivo--->PID:%d--->FD:%d--->Bytes:%d",pid,fileDescriptor,tamanioALeer);

		_Bool verificaPid(t_indiceTablaProceso* entrada){
						return entrada->pid == pid;
					}

		_Bool verificaFd(t_entradaTablaProceso* entrada){
			return entrada->fd == fileDescriptor;
		}


		pthread_mutex_lock(&mutexListaTablaArchivos);
		int fileDescriptorAbierto=verificarFileDescriptorAbierto(pid,fileDescriptor);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		if(!fileDescriptorAbierto){
			excepcionFileDescriptorNoAbierto(socket,pid);
			return;
		}

		pthread_mutex_lock(&mutexListaTablaArchivos);
		t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
		t_entradaTablaProceso* entrada = list_remove_by_condition(entradaTablaProceso->tablaProceso,(void*)verificaFd);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		int cursor=entrada->puntero;

		int tiene_permisoLectura=0;
		char *permiso_lectura = "r";
		if(string_contains(entrada->flags, permiso_lectura)){
			tiene_permisoLectura=1;
		}
		pthread_mutex_lock(&mutexListaTablaArchivos);
		list_add(entradaTablaProceso->tablaProceso,entrada);
		list_add(listaTablasProcesos,entradaTablaProceso);
		pthread_mutex_unlock(&mutexListaTablaArchivos);

		if(!tiene_permisoLectura){
			excepcionPermisosLectura(socket,pid);
			return;
		}
		pthread_mutex_lock(&mutexTablaGlobal);
		char* direccion = buscarDireccionEnTablaGlobal(entrada->globalFd);
		pthread_mutex_unlock(&mutexTablaGlobal);

		printf("Direccion:%s\n",direccion);

		char** array_dir=string_n_split(direccion, 12, "/");
		char* nombreArchivo=array_dir[0];
		int tamanoNombre=sizeof(char)*strlen(nombreArchivo);



		printf("Tamano del nombre del archivo:%d\n",tamanoNombre);
		printf("Nombre del archivo:%s\n",nombreArchivo);
		printf("Puntero :%d\n",cursor);
		printf("Tamano a leer :%d\n",tamanioALeer);


		pthread_mutex_lock(&mutexFS);
		send(socketFyleSys,&comandoLeer,sizeof(char),0);
		send(socketFyleSys,&tamanoNombre,sizeof(int),0);
		send(socketFyleSys,nombreArchivo,tamanoNombre,0);
		send(socketFyleSys,&cursor,sizeof(int),0);
		send(socketFyleSys,&tamanioALeer,sizeof(int),0);

		recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);

		printf("Resultado de ejecucion : %d\n",resultadoEjecucion);

		if(resultadoEjecucion<0){
					pthread_mutex_unlock(&mutexFS);
					excepcionFileSystem(socket,pid);
					return;
					}
		informacion = malloc(tamanioALeer);
		recv(socketFyleSys,informacion,tamanioALeer,0);
		pthread_mutex_unlock(&mutexFS);

		printf("Informacion leida :%s\n",informacion);

	send(socket,&resultadoEjecucion,sizeof(int),0);
	send(socket,informacion,tamanioALeer,0);

	free(informacion);
	log_info(loggerConPantalla,"Datos obtenidos--->PID:%d--->Datos:%s",pid,informacion);
	free(data);
}

void moverCursorArchivo(int socket){
		int pid;
		int fileDescriptor;
		int posicion;
		int resultadoEjecucion ;
		recv(socket,&pid,sizeof(int),0);
		recv(socket,&fileDescriptor,sizeof(int),0);
		recv(socket,&posicion,sizeof(int),0);

		log_info(loggerConPantalla,"Moviendo puntero--->PID:%d--->FD:%d--->Posicion:%d",pid,fileDescriptor,posicion);

		_Bool verificaPid(t_indiceTablaProceso* entrada){
						return entrada->pid == pid;
					}

		_Bool verificaFd(t_entradaTablaProceso* entrada){
						return entrada->fd == fileDescriptor;
					}

		//verificar que la tabla de ese pid exista

			int fileDescriptorAbierto; //seguir desde aca
			t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
					if(list_any_satisfy(entradaTablaProceso->tablaProceso,(void*)verificaFd)) fileDescriptorAbierto = 1;
					else fileDescriptorAbierto = 0;

			if(!fileDescriptorAbierto){
						excepcionArchivoInexistente(socket,pid);
			}else{

				t_entradaTablaProceso* entrada = list_remove_by_condition(entradaTablaProceso->tablaProceso,(void*)verificaFd);
				entrada->puntero = posicion;
				list_add(entradaTablaProceso->tablaProceso,entrada);
				list_add(listaTablasProcesos,entradaTablaProceso);

				/*TODO: Falta codear esta funcion en FS*/
				//hacer los sends para que el FS verifique que esa posicion existe dentro del archivo
				//si recibo 0 siginfica que algo anda mal
				//si recibo 1 significa que esta todo ok

				resultadoEjecucion=1;
				send(socket,&resultadoEjecucion,sizeof(int),0);
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



int actualizarTablaDelProceso(int pid,char* flags,int indiceEnTablaGlobal){
	log_info(loggerConPantalla,"Agregando entrada a tabla por proceso");

	_Bool verificaPid(t_indiceTablaProceso* entrada){
		return entrada->pid == pid;
	}

		 t_indiceTablaProceso* entradaTablaExistente = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);//la remuevo para actualizarlo
		 t_entradaTablaProceso* entrada = malloc(sizeof(t_entradaTablaProceso));
		 entrada->fd = entradaTablaExistente->tablaProceso->elements_count + 3;
		 entrada->flags = flags;
		 entrada->globalFd = indiceEnTablaGlobal;
		 entrada->puntero = 0;
		 printf("Agrego el indice :%d\n",entrada->globalFd);
		 list_add(entradaTablaExistente->tablaProceso,entrada);
		 list_add(listaTablasProcesos,entradaTablaExistente);//la vuelvo a agregar a la lista

		 return entrada->fd;
}

int borrarEntradaTablaProceso(int pid,int fd){
	log_info(loggerConPantalla,"Borrando entrada en tabla archivos--->PID:%d--->FD:%d",pid,fd);
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

void borrarEntradaEnTablaGlobal(int indiceTablaGlobal){
	list_remove_and_destroy_element(tablaArchivosGlobal,indiceTablaGlobal,free);
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

void disminuirOpenYVerificarExistenciaEntradaGlobal(int indiceTablaGlobal){
	log_info(loggerConPantalla,"Verificando apertura en tabla global--->Indice:%d",indiceTablaGlobal);
	t_entradaTablaGlobal* entrada = list_get(tablaArchivosGlobal,indiceTablaGlobal);
	entrada->open --;

	if(entrada->open==0){
		list_remove(tablaArchivosGlobal,indiceTablaGlobal);
		if(tablaArchivosGlobal->elements_count > 0)actualizarIndicesGlobalesEnTablasProcesos(indiceTablaGlobal); /*TODO: Esta jodiedno esta*/
		free(entrada);
	}
}

void actualizarIndicesGlobalesEnTablasProcesos(int indiceTablaGlobal){
	log_info(loggerConPantalla,"Actualizando indices globales mayores al indice eliminado:%d",indiceTablaGlobal);
	int i;
	int j;
	t_indiceTablaProceso* indiceTabla;
	t_entradaTablaProceso* entrada;
	for(i=0;listaTablasProcesos->elements_count;i++){
		indiceTabla = list_get(listaTablasProcesos,i);

		for(j=0;j<indiceTabla->tablaProceso->elements_count;j++){
			entrada = list_get(indiceTabla->tablaProceso,j);
			if(entrada->globalFd > indiceTablaGlobal) entrada->globalFd--; /*TODO: No pregunto si es igual porque se supone que no existe mas en esa tabla. Corroborarlo*/
		}
	}
}

int buscarIndiceEnTablaGlobal(char* direccion){
	log_info(loggerConPantalla,"Buscando indice en tabla global--->Direccion:%s",direccion);
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
	log_info(loggerConPantalla,"Buscando direccion en tabla global--->Indice:%d",indice);
	t_entradaTablaGlobal* entrada = list_get(tablaArchivosGlobal,indice);

	return entrada->path;
}

int verificarArchivoAbiertoEnTablaGlobal(int indiceTablaGlobal){

	int resultado;

	t_entradaTablaGlobal* entrada = list_get(tablaArchivosGlobal,indiceTablaGlobal);

	if(entrada->open > 1) resultado = -1;
	else resultado = 1;
	return resultado;
}


int buscarIndiceGlobalEnTablaProceso(int pid,int fileDescriptor){

	_Bool verificaPid(t_indiceTablaProceso* entrada){
									return entrada->pid == pid;
								}


	_Bool verificaFd(t_entradaTablaProceso* entrada){
				return entrada->fd == fileDescriptor;
			}

	t_indiceTablaProceso* indice = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
	t_entradaTablaProceso* entrada = list_remove_by_condition(indice->tablaProceso,(void*)verificaFd);

	int indiceTablaGlobal = entrada->globalFd;

	list_add(indice->tablaProceso,entrada);
	list_add(listaTablasProcesos,indice);

	return indiceTablaGlobal;
}

void inicializarTablaProceso(int pid){
	 t_indiceTablaProceso* indiceNuevaTabla = malloc(sizeof(t_indiceTablaProceso));
	 indiceNuevaTabla->pid = pid;
	 indiceNuevaTabla->tablaProceso = list_create();
	 list_add(listaTablasProcesos,indiceNuevaTabla);
}

int verificarFileDescriptorAbierto(int pid,int fileDescriptor){
	int resultado;
	_Bool verificaPid(t_indiceTablaProceso* entrada){
								return entrada->pid == pid;
							}

	_Bool verificaFd(t_entradaTablaProceso* entrada){
						return entrada->fd == fileDescriptor;
					}

		t_indiceTablaProceso* entradaTablaProceso = list_remove_by_condition(listaTablasProcesos,(void*)verificaPid);
		if(list_any_satisfy(entradaTablaProceso->tablaProceso,(void*)verificaFd)) resultado = 1;
		else resultado = 0;
		list_add(listaTablasProcesos,entradaTablaProceso);
		return resultado;
}

int validarArchivo(char* ruta){
	log_info(loggerConPantalla,"Validando que el archivo exista--->Ruta:%s",ruta);
	char ordenValidarArchivo = 'V';
	int tamano=sizeof(char)*strlen(ruta);
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
					case 'A':	interfaceAbrirArchivo(socket);
								break;
					case 'B':	borrarArchivo(socket); /*TODO: Falta eliminar las entradas en TODAS las tablas y en la global. Ademas actualizar las tablas de los procesos y corroborar si alguien lo tiene abierto*/
								break;
					case 'O':	interfaceLeerArchivo(socket); /*TODO: Falta saber que mandarle a FS*/
								break;
					case 'G': 	interfaceEscribirArchivo(socket); /*TODO: Falta testear*/
								break;
					case 'P':	cerrarArchivo(socket); /*TODO: Falta testeas*/
								break;
					case 'M':	moverCursorArchivo(socket); /*TODO: Falta desarrollar la validacion del tamano del archivo en FS*/
								break;
					default:
					log_error(loggerConPantalla ,"Orden no reconocida: %c",orden);
					break;
				}
		log_info(loggerConPantalla,"Finalizando atencion de Interfaz Handler de File System");
}

void interfaceAbrirArchivo(int socket){
	pthread_t IOthread;

	t_fsAbrir* data = malloc(sizeof(t_fsAbrir));

	recv(socket,&data->pid,sizeof(int),0);

	recv(socket,&data->tamanoDireccion,sizeof(int),0);
	data->direccion = malloc(data->tamanoDireccion*sizeof(char) + sizeof(char));

	recv(socket,data->direccion,data->tamanoDireccion,0);
	strcpy(data->direccion + data->tamanoDireccion, "\0");

	recv(socket,&data->tamanoFlags,sizeof(int),0);
	data->flags= malloc(data->tamanoFlags*sizeof(char)+sizeof(char));

	recv(socket,data->flags,data->tamanoFlags,0);
	strcpy(data->flags + data->tamanoFlags, "\0");


	data->socket =socket;

	pthread_create(&IOthread,NULL,(void*)abrirArchivo,data);
}

void interfaceEscribirArchivo(int socket){

	pthread_t IOthread;

		t_fsEscribir* data = malloc(sizeof(t_fsEscribir));

		recv(socket,&data->pid,sizeof(int),0);

		recv(socket,&data->fd,sizeof(int),0);

		recv(socket,&data->size,sizeof(int),0);
		data->informacion=malloc(data->size);

		recv(socket,data->informacion,data->size,0);

		data->socket =socket;

		pthread_create(&IOthread,NULL,(void*)escribirArchivo,data);
}

void interfaceLeerArchivo(int socket){
	pthread_t IOthread;

	t_fsLeer* data = malloc(sizeof(t_fsLeer));

	recv(socket,&data->pid,sizeof(int),0);

	recv(socket,&data->fd,sizeof(int),0);

	recv(socket,&data->size,sizeof(int),0);

	data->socket=socket;

	pthread_create(&IOthread,NULL,(void*)leerArchivo,data);
}

void testEscribirArchivo(){
	char comandoGuardarDatos = 'G';
	char* nombreArchivo = "cadenas.bin";
	int tamanoNombre = strlen(nombreArchivo);
	int cursor=0;
	char* informacion=string_new();

	printf("Ingrese informacion a escribir\n");
	scanf("%s",informacion);


	int size=strlen(informacion)*sizeof(int);
	send(socketFyleSys,&comandoGuardarDatos,sizeof(char),0);
	send(socketFyleSys,&tamanoNombre,sizeof(int),0);
	send(socketFyleSys,nombreArchivo,tamanoNombre,0);
	send(socketFyleSys,&cursor,sizeof(int),0);
	send(socketFyleSys,&size,sizeof(int),0);
	send(socketFyleSys,informacion,size,0);

	recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);
	printf("Resultado Ejecucion:%d\n",resultadoEjecucion);
	sleep(5);
}

void testLeerArchivo(){

	char comandoObtenerDatos = 'O';
	char* nombreArchivo = "cadenas.bin";
	int tamanoNombre = strlen(nombreArchivo);
	int cursor=0;
	char* informacion;
	int size=16;

		send(socketFyleSys,&comandoObtenerDatos,sizeof(char),0);
		send(socketFyleSys,&tamanoNombre,sizeof(int),0);
		send(socketFyleSys,nombreArchivo,tamanoNombre,0);
		send(socketFyleSys,&cursor,sizeof(int),0);
		send(socketFyleSys,&size,sizeof(int),0);

		informacion = malloc(size);

		recv(socketFyleSys,informacion,size,0);
		printf("Informacion recibida:%s\n",informacion);
		sleep(5);

}



#endif /* CAPAFILESYSTEM_H_ */
