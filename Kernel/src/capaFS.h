#ifndef _CAPAFS_
#define _CAPAFS_
#include "sockets.h"
#include <commons/collections/list.h>

//EN GENERAL FALTAN SENDS Y RECVS AL FS PARA SABER SI EL RESULTADO DE EJECUCION ESTA BIEN
//Y QUE EL FS HAGA LO QUE TENGA QUE HACER SIN PROBLEMAS

//--------Capa FS--------//
int contadorFilasTablaGlobal=0;
char** tablaGlobalArchivos;
typedef struct FS{//Para poder guardar en la lista
	int pid;
	char** tablaArchivoPorProceso;
	int contadorFilasTablaPorProceso;
}t_tablaArchivoPorProceso;
t_list* listaTablasArchivosPorProceso;

typedef struct{
	char* path;
	int open;
}t_entradaTablaGlobal;
t_list* listaTablaArchivosGlobal;

void agregarEntradaEnTablaGlobal(char* direccion,int sizeDireccion);
int buscarIndiceEnTablaProceso(int pid,int*indicePid);


int validarArchivoFS(char* ruta){
	log_info(loggerConPantalla,"Validando que el archivo exista--->Ruta:%s",ruta);
	char orden = 'V';
	int tamano=sizeof(int)*strlen(ruta);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,ruta,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);

	log_info(loggerConPantalla,"Archivo validado--->Ruta:%s",ruta);
	return validado;
}

int crearArchivoFS(int socket_aceptado, char* direccion ){
	log_info(loggerConPantalla,"Creando archivo en FileSystem---> %s",direccion);

	int validado;

	char** array_dir=string_n_split(direccion, 12, "/");
	   int d=0;
	   while(!(array_dir[d] == NULL)){
	      d++;
	   }
	char* nombreArchivo=array_dir[d];
	int tamanoNombre=sizeof(int)*strlen(nombreArchivo);
	char orden = 'C';
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamanoNombre,sizeof(int),0);
	send(socketFyleSys,nombreArchivo,tamanoNombre,0);

	recv(socketFyleSys,&validado,sizeof(int),0);

	log_info(loggerConPantalla,"Archivo creado en FileSystem---> %s",direccion);
	return validado;

}

void moverCursorArchivoFS(int socket_aceptado){//SIN TERMINAR , faltan los sends y recv al FS
	log_info(loggerConPantalla,"Moviendo puntero");
	int pid;
	int descriptorArchivo;
	int posicion;
	int resultadoEjecucion ;
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	recv(socket_aceptado,&posicion,sizeof(int),0);

	//verificar que la tabla de ese pid exista
	int indicePid;
	int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);


		int j,i,posicion;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<4 ; j++)
		   {
			   if(tablaAVer->tablaArchivoPorProceso[i][2]==descriptorArchivo){
				   encontro=1;
				   posicion=i;
			   }
		   }
		}

		if(encontro==0){
			resultadoEjecucion=0;
			excepcionArchivoInexistente(socket_aceptado,pid);
			return;
		}else{

			tablaAVer->tablaArchivoPorProceso[i][3]=posicion;

			/*TODO: No encuentro la funcion en FS*/
			//hacer los sends para que el FS verifique que esa posicion existe dentro del archivo
			//si recibo 0 siginfica que algo anda mal
			//si recibo 1 significa que esta todo ok

			resultadoEjecucion=1;
		}

	}

	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void borrarArchivoFS(int socket_aceptado){//SIN TERMINAR
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion ;
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	log_info(loggerConPantalla,"Borrando un archivo--->PID:%d",pid);

	//verificar que la tabla de ese pid exista
	int indicePid;
	int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);


	if(!tablaExiste){
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);


		int j,i,posicion;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<4 ; j++)
		   {
			   if(tablaAVer->tablaArchivoPorProceso[i][2]==descriptorArchivo){
				   encontro=1;
				   posicion=i;
			   }
		   }
		}

		if(encontro==0){/*TODO: No entiendo esta situacion*/
			resultadoEjecucion=0;
			//excepcion(-1,socket_aceptado);//No encontro en tabla
		}else{
			//borrar de la tabla de archivo por proceso
			//borrar de la tabla global
			//hacer los sends para que el FS borre ese archivo y deje los bloques libres
			char comandoBorrarArchivo='B';
			send(socketFyleSys,comandoBorrarArchivo,sizeof(char),0);
			//send(socketFyleSys,&tamanioArchivo,sizeof(int),0); TODO: Encontrar el size del file
			//send(socketFyleSys,nombreArchivo,strlen(nombreArchivo)*sizeof(char),0); TODO: Encontrar el nombre del archivo
			recv(socketFyleSys,&resultadoEjecucion,sizeof(char),0);
				if(resultadoEjecucion < 0) {
					excepcionFileSystem(socket_aceptado,pid);
					return;
				}
		}

	}
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
	log_info(loggerConPantalla,"Archivo borrado--->PID:%d",pid);
}

void cerrarArchivoFS(int socket_aceptado){//SIN TERMINAR
	log_info(loggerConPantalla,"Cerrando el archivo indicado");
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion ;
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);

	//verificar que la tabla de ese pid exista
	int indicePid;
	int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);


	if(tablaExiste==0){
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);


		int j,i,posicion;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<4 ; j++)
		   {
			   if(tablaAVer->tablaArchivoPorProceso[i][2]==descriptorArchivo){
				   encontro=1;
				   posicion=i;
			   }
		   }
		}

		if(encontro==0){//no encontro en tabla
			excepcionArchivoInexistente(socket_aceptado,pid);
			return;
			//resultadoEjecucion=0;
			//excepcion(-1,socket_aceptado);
		}else{
			//borrar de la tabla de archivo por proceso
			//borrar de la tabla global


			resultadoEjecucion=1;
		}

	}

	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void obtenerArchivoFS(int socket_aceptado){//SIN TERMINAR
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion;
	int informacionPunteroARecibir;
	int tamanioDeLaInstruccionEnBytes;//vendria a ser un offset
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	recv(socket_aceptado,&informacionPunteroARecibir,sizeof(int),0);
	recv(socket_aceptado,&tamanioDeLaInstruccionEnBytes,sizeof(int),0);
	log_info(loggerConPantalla,"Obteniendo datos del archivo indicado---PID:%d",pid);

	//verificar que la tabla de ese pid exista
	int indicePid;
	int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);


	if(tablaExiste==0){
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);


		int j,i,posicion;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<4 ; j++)
		   {
			   if(tablaAVer->tablaArchivoPorProceso[i][2]==descriptorArchivo){
				   encontro=1;
				   posicion=i;
			   }
		   }
		}

		if(encontro==0){//no encontro en tabla
			excepcionArchivoInexistente(socket_aceptado,pid);
			return;
		}else{
			int tiene_permisoLectura=0;
			const char *permiso_lectura = "r";
			if(string_contains(tablaAVer->tablaArchivoPorProceso[i][0], permiso_lectura)){
				tiene_permisoLectura=1;
			}
			if(!tiene_permisoLectura){
				excepcionPermisosLectura(socket_aceptado,pid);
				return;
			}

			int punteroADondeVaALeer=tablaAVer->tablaArchivoPorProceso[i][3]+informacionPunteroARecibir;

			//TODO: sends a FS mandandole el puntero y el offset , y que me devuelva 0 si no encontro puntero , y 1
			recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);
			if(resultadoEjecucion<0){
					excepcionFileSystem(socket_aceptado,pid);
					return;
				}
			}

		}
	//TODO: si salio todo bien y mando la info leida al CPU
		send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
		log_info(loggerConPantalla,"Datos obtenidos---PID:%d",pid);
	}


void guardarArchivoFS(int socket_aceptado){//SIN TERMINAR
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion;
	int informacionPunteroARecibir;
	int tamanioDeLaInstruccionEnBytes;//vendria a ser un offset
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	recv(socket_aceptado,&tamanioDeLaInstruccionEnBytes,sizeof(int),0);

	char* buffer = malloc(tamanioDeLaInstruccionEnBytes);
    recv(socket_aceptado,buffer,tamanioDeLaInstruccionEnBytes,0);
    strcpy(buffer + tamanioDeLaInstruccionEnBytes, "\0");
	log_info(loggerConPantalla,"Guardando datos del archivo indicado--->PID:%d--->Datos:%s",pid,buffer);

	//verificar que la tabla de ese pid exista
		int indicePid;
		int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);


	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcionArchivoInexistente(socket_aceptado,pid);
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);


		int j,i,posicion;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<4 ; j++)
		   {
			   if(tablaAVer->tablaArchivoPorProceso[i][2]==descriptorArchivo){
				   encontro=1;
				   posicion=i;
			   }
		   }
		}

		if(encontro==0){/*TODO: No veo esta situacion*/
			resultadoEjecucion=0;
			//excepcion(-1,socket_aceptado);//no encontro en tabla
		}else{


			int tiene_permisoEscritura=0;
			const char *permiso_escritura = "w";
			if(string_contains(tablaAVer->tablaArchivoPorProceso[i][0], permiso_escritura)){
				tiene_permisoEscritura=1;
			}

			if(!tiene_permisoEscritura){
				excepcionPermisosEscritura(socket_aceptado,pid);
				return;
			}

				int punteroADondeVaALeer=tablaAVer->tablaArchivoPorProceso[i][3];

				//sends a FS mandandole el puntero y el offset , y que me devuelva 0 si no encontro puntero , y 1
				//si salio todo bien y mando la info leida al CPU


				char** array_dir=string_n_split(tablaGlobalArchivos[tablaAVer->tablaArchivoPorProceso[i][1]][0], 12, "/");
				   int d=0;
				   while(!(array_dir[d] == NULL)){
				      d++;
				   }
				char* nombreArchivo=array_dir[d];
				int tamanoArchivoAMandar=sizeof(int)*strlen(nombreArchivo);

				send(socketFyleSys,&tamanoArchivoAMandar,sizeof(int),0);
				send(socketFyleSys,nombreArchivo,tamanoArchivoAMandar,0);
				send(socketFyleSys,&punteroADondeVaALeer,sizeof(int),0);
				send(socketFyleSys,&tamanioDeLaInstruccionEnBytes,sizeof(int),0);
				send(socketFyleSys,buffer,tamanioDeLaInstruccionEnBytes,0);
//recvs

			resultadoEjecucion=1;
		}

	}
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

int agregarATablaPorProcesoYDevolverDescriptor(char* flags, int i){
	int descriptorADevolver;
	//agregarlo en la tabla del proceso
	//y agregarlo con el flag y el file descriptor y el indice hacia la tabla global

	//verificar que la tabla de ese pid exista
	int indicePid;
	int tablaExiste=buscarIndiceEnTablaProceso(pid,&indicePid);

	 if(tablaExiste==0){
		//tabla no existe
		t_tablaArchivoPorProceso* tablaAAgregar = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAAgregar->pid=pid;
		tablaAAgregar->tablaArchivoPorProceso;
		tablaAAgregar->contadorFilasTablaPorProceso=0;
		list_add(listaTablasArchivosPorProceso,tablaAAgregar);
		int ultimoElemento=list_size(listaTablasArchivosPorProceso)-1;
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,ultimoElemento);

		tablaAVer->tablaArchivoPorProceso[0][0]=flags;
		tablaAVer->tablaArchivoPorProceso[0][1]=tablaGlobalArchivos[i][3];//apunta al indice de la global
		tablaAVer->tablaArchivoPorProceso[0][2]=3;//FileDescriptor siempre empieza en 3
		tablaAVer->tablaArchivoPorProceso[0][3]=0;//Puntero siempre empieza en 0

		descriptorADevolver=3;

	}else{
		//tabla existe
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,indicePid);
		tablaAVer->contadorFilasTablaPorProceso++;
		tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][0]=flags;
		tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][1]=tablaGlobalArchivos[i][3];//apunta al indice de la global
		tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2]=tablaAVer->contadorFilasTablaPorProceso+3;//FileDescriptor siempre empieza en 3
		tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][3]=0;//Puntero siempre empieza en 0

		descriptorADevolver=tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2];
	}

	return descriptorADevolver;
}

void abrirArchivoEnTablas(int socket_aceptado){
	log_info(loggerConPantalla,"Abriendo un archivo en las tablas");
	int pid;
	int tamanoDireccion;
	int tamanoFlags;
	char* direccion;
	int descriptorADevolver;
	char* flags;
	recv(socket_aceptado,&pid,sizeof(int),0);
	printf("PID:%d\n",pid);
	recv(socket_aceptado,&tamanoDireccion,sizeof(int),0);
	printf("Tamano direccion%d\n",tamanoDireccion);
	direccion = malloc(tamanoDireccion);
	recv(socket_aceptado,direccion,tamanoDireccion,0);
	strcpy(direccion + tamanoDireccion, "\0");
	printf("Direccion %s\n",direccion);
	recv(socket_aceptado,&tamanoFlags,sizeof(int),0);
	printf("Tamano flags%d\n",tamanoFlags);
	flags= malloc(tamanoFlags);
	recv(socket_aceptado,flags,tamanoFlags,0);
	printf("Flags %s",flags);
	strcpy(flags + tamanoFlags, "\0");

	int archivoExistente=validarArchivoFS(direccion);
	int tiene_permisoCreacion=0;
	char permiso_creacion = 'c';
	if(string_contains(flags,&permiso_creacion)){
		tiene_permisoCreacion=1;
	}

	if(!archivoExistente && !tiene_permisoCreacion){ // No existe el archivo y no tiene permisos
				excepcionPermisosCrear(socket_aceptado,pid);
				free(direccion);
				free(flags);
				return;
	}


	if(archivoExistente==1){
		//me fijo que exista en la tabla global de archivos
		int i,j;
		int entradaGlobalExistente=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<3 ; j++)
		   {

		      if(strcmp(tablaGlobalArchivos[i][0],direccion) == 0){
		    	  entradaGlobalExistente=1;
		    	  tablaGlobalArchivos[i][1]=tablaGlobalArchivos[i][1]+1;//aumento el open
		    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,i);
		      }
		   }
		}

		if(tiene_permisoCreacion && !entradaGlobalExistente){

		}

		if(!entradaGlobalExistente){
			//Agrego a la tabla global
				agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);
	    	//agrego en la tabla del proceso
	    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,contadorFilasTablaGlobal);

		}

		int validado=1;
		send(socket_aceptado,&validado,sizeof(int),0);
		send(socket_aceptado,&descriptorADevolver,sizeof(int),0);

	}else if(tiene_permisoCreacion==1){

		int validacionCrear=crearArchivoFS(socket_aceptado,direccion);
		if(validacionCrear==0){
			excepcionFileSystem(socket_aceptado,pid);
			free(direccion);
			free(flags);
			return;
		}
		agregarEntradaEnTablaGlobal(direccion,tamanoDireccion);

  	  //agregarlo en la tabla del proceso
  	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,contadorFilasTablaGlobal);

  	  int validado=1;
  	  send(socket_aceptado,&validado,sizeof(int),0);
  	  send(socket_aceptado,&descriptorADevolver,sizeof(int),0);
	}
}


void interfazHandlerParaFileSystem(char orden,int socket_aceptado){
		log_info(loggerConPantalla,"Iniciando Interfaz Handler Para File System");

		switch(orden){
				case 'A'://abrir archivo
					abrirArchivoEnTablas(socket_aceptado);
					break;
				case 'V'://validar archivo
					validarArchivoFS("alumno.bin");
					break;
				case 'B'://borrar archivo
					borrarArchivoFS(socket_aceptado);
					break;
				case 'O'://obtener datos
					obtenerArchivoFS(socket_aceptado);
					break;
				case 'G'://guardar archivo
					guardarArchivoFS(socket_aceptado);
					break;
				case 'P'://guardar archivo
					cerrarArchivoFS(socket_aceptado);
					break;
				case 'M'://guardar archivo
					moverCursorArchivoFS(socket_aceptado);
					break;
			default:
				if(orden == '\0') break;
				log_error(loggerConPantalla ,"Orden no reconocida: %c",orden);
				break;
			}
			orden = '\0';
			log_info(loggerConPantalla,"Finalizando atencion de Interfaz Handler de File System");
			return;

}


void agregarEntradaEnTablaGlobal(char* direccion,int sizeDireccion){
	  contadorFilasTablaGlobal++;
	  tablaGlobalArchivos[contadorFilasTablaGlobal][0]= malloc(sizeDireccion);
	  strcpy(tablaGlobalArchivos[contadorFilasTablaGlobal][0],direccion);
	  tablaGlobalArchivos[contadorFilasTablaGlobal][1]=1;//el open
	  tablaGlobalArchivos[contadorFilasTablaGlobal][2]=contadorFilasTablaGlobal;
}

int buscarIndiceEnTablaProceso(int pid,int* indice){
	int k;
	int resultado=0;
	t_tablaArchivoPorProceso* tabla;
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
			tabla = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
			if(tabla->pid==pid){
				resultado=1;
				*indice=k;
			}
		}
	return resultado;

}

#endif
