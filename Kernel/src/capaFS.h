#ifndef _CAPAFS_
#define _CAPAFS_
#include "sockets.h"

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


void excepcion(int codigoError,int socket){

	switch(codigoError){

		case -1:

			break;

		case -2:

			break;

		case -3:

			break;

	}
}

int validarArchivoFS(char* ruta){
	log_info(loggerConPantalla,"Validadno que el archivo indicado exista");
	char orden = 'V';
	int tamano=sizeof(int)*strlen(ruta);
	int validado;
	send(socketFyleSys,&orden,sizeof(char),0);
	send(socketFyleSys,&tamano,sizeof(int),0);
	send(socketFyleSys,ruta,tamano,0);
	recv(socketFyleSys,&validado,sizeof(int),0);

	//printf("\n \n %d \n \n ",validado);
	return validado;
}

int crearArchivoFS(int socket_aceptado, char* direccion ){

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

	int k;
	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);


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
	log_info(loggerConPantalla,"Borrando el archivo indicado");
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion ;
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);

	int k;
	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);


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
			excepcion(-1,socket_aceptado);//No encontro en tabla
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
					excepcionArchivoInexistente(socket_aceptado,pid);
					return;
				}
		}

	}
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void cerrarArchivoFS(int socket_aceptado){//SIN TERMINAR
	log_info(loggerConPantalla,"Cerrando el archivo indicado");
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion ;
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);

	int k;
	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcion(-1,socket_aceptado);//no encontro en tabla
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);


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
	log_info(loggerConPantalla,"Obteniendo datos del archivo indicado");
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion;
	int informacionPunteroARecibir;
	int tamanioDeLaInstruccionEnBytes;//vendria a ser un offset
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	recv(socket_aceptado,&informacionPunteroARecibir,sizeof(int),0);
	recv(socket_aceptado,&tamanioDeLaInstruccionEnBytes,sizeof(int),0);


	int k;
	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcionArchivoInexistente(socket_aceptado,pid);
		return;
		//excepcion(-1,socket_aceptado);//no encontro en tabla
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);


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


			if(tiene_permisoLectura==1){
				int punteroADondeVaALeer=tablaAVer->tablaArchivoPorProceso[i][3]+informacionPunteroARecibir;

				//TODO: sends a FS mandandole el puntero y el offset , y que me devuelva 0 si no encontro puntero , y 1
				recv(socketFyleSys,&resultadoEjecucion,sizeof(int),0);
				if(resultadoEjecucion<0){
					excepcionFileSystem(socket_aceptado,pid);
					return;
				}
			}else{
				//No tiene permiso de lectura para ejecutar esta instruccion
				excepcionPermisosLectura(socket_aceptado,pid);
				return;
			}

		}

	}
	//TODO: si salio todo bien y mando la info leida al CPU
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void guardarArchivoFS(int socket_aceptado){//SIN TERMINAR
	log_info(loggerConPantalla,"Guardando datos del archivo indicado");
	int pid;
	int descriptorArchivo;
	int resultadoEjecucion;
	int informacionPunteroARecibir;
	int tamanioDeLaInstruccionEnBytes;//vendria a ser un offset
	recv(socket_aceptado,&pid,sizeof(int),0);
	recv(socket_aceptado,&descriptorArchivo,sizeof(int),0);
	recv(socket_aceptado,&tamanioDeLaInstruccionEnBytes,sizeof(int),0);

    char* bufferr = malloc(tamanioDeLaInstruccionEnBytes);
    recv(socket_aceptado,bufferr,tamanioDeLaInstruccionEnBytes,0);
    strcpy(bufferr + tamanioDeLaInstruccionEnBytes, "\0");


	//falta recv del buffer

	int k;
	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}




	if(tablaExiste==0){
		resultadoEjecucion=0;
		excepcion(-1,socket_aceptado);//no encontro en tabla
	}else{
		t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);


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
			excepcion(-1,socket_aceptado);//no encontro en tabla
		}else{


			int tiene_permisoEscritura=0;
			const char *permiso_escritura = "w";
			if(string_contains(tablaAVer->tablaArchivoPorProceso[i][0], permiso_escritura)){
				tiene_permisoEscritura=1;
			}


			if(tiene_permisoEscritura==1){
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
				send(socketFyleSys,bufferr,tamanioDeLaInstruccionEnBytes,0);


//recvs

			}else{
				resultadoEjecucion=0;
				//retornar codigo de error o algo asi
				excepcion(-3,socket_aceptado);//no tiene permiso de escritura
			}



			resultadoEjecucion=1;
		}

	}
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

int agregarATablaPorProcesoYDevolverDescriptor(char* flags, int i){
	int k;
	int descriptorADevolver;
	//agregarlo en la tabla del proceso
	//y agregarlo con el flag y el file descriptor y el indice hacia la tabla global

	t_tablaArchivoPorProceso* tablaAVerificar = malloc(sizeof(t_tablaArchivoPorProceso));
	int tablaExiste=0;
	int dondeEstaElPid;
	//verificar que la tabla de ese pid exista
	for(k=0;k<listaTablasArchivosPorProceso->elements_count;k++){
		tablaAVerificar  = (t_tablaArchivoPorProceso*) list_get(listaTablasArchivosPorProceso,k);
		if(tablaAVerificar->pid==pid){
			tablaExiste=1;
			dondeEstaElPid=k;
		}
	}

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
		tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);
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
	printf("%d\n",pid);
	recv(socket_aceptado,&tamanoDireccion,sizeof(int),0);
	printf("%d\n",tamanoDireccion);
	direccion = malloc(tamanoDireccion);
	recv(socket_aceptado,direccion,tamanoDireccion,0);
	strcpy(direccion + tamanoDireccion, "\0");
	printf("%s\n",direccion);
	recv(socket_aceptado,&tamanoFlags,sizeof(int),0);
	printf("%d\n",tamanoFlags);
	flags= malloc(tamanoFlags);
	recv(socket_aceptado,flags,tamanoFlags,0);
	printf("%s",flags);
	printf("Recibi todo\n");

	int elArchivoExiste=validarArchivoFS(direccion);
	int tiene_permisoCreacion=0;
	char permiso_creacion = 'c';
	if(string_contains(flags,&permiso_creacion)){
		tiene_permisoCreacion=1;
	}


	if(elArchivoExiste==1){

		//me fijo que exista en la tabla global de archivos
		int j,i,k;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<3 ; j++)
		   {

		      if(strcmp(tablaGlobalArchivos[i][0],direccion) == 0){
		    	  encontro=1;
		    	  tablaGlobalArchivos[i][1]=tablaGlobalArchivos[i][1]+1;//aumento el open

		    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,i);
		      }
		   }
		}

		if(tiene_permisoCreacion==1 && encontro==0){

		}

		if(encontro==0){
	    	  contadorFilasTablaGlobal++;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][0]=direccion;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][1]=1;//el open
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][2]=contadorFilasTablaGlobal;

	    	  //agregarlo en la tabla del proceso
	    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,contadorFilasTablaGlobal);

		}

		int validado=1;
		send(socket_aceptado,&validado,sizeof(int),0);
		send(socket_aceptado,&descriptorADevolver,sizeof(int),0);

	}else if(tiene_permisoCreacion==1){

		int validacionCrear=crearArchivoFS(socket_aceptado,direccion);
		if(validacionCrear==0){
			excepcionArchivoInexistente(socket_aceptado,pid);
			return;
		}

  	  contadorFilasTablaGlobal++;
  	  tablaGlobalArchivos[contadorFilasTablaGlobal][0]=direccion;
  	  tablaGlobalArchivos[contadorFilasTablaGlobal][1]=1;//el open
  	  tablaGlobalArchivos[contadorFilasTablaGlobal][2]=contadorFilasTablaGlobal;

  	  //agregarlo en la tabla del proceso
  	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,contadorFilasTablaGlobal);

  	  int validado=1;
  	  send(socket_aceptado,&validado,sizeof(int),0);
  	  send(socket_aceptado,&descriptorADevolver,sizeof(int),0);
	}else{
		//no encontro en tabla. No tiene permisos para crear
		excepcionPermisosCrear(socket_aceptado,pid);
		return;
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
#endif
