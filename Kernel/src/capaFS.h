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




int validarArchivoFS(char* ruta){
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

void moverCursorArchivoFS(int socket_aceptado){//SIN TERMINAR , faltan los sends y recv al FS
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
		}else{

			tablaAVer->tablaArchivoPorProceso[i][3]=posicion;

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
		}else{
			//borrar de la tabla de archivo por proceso
			//borrar de la tabla global

			//hacer los sends para que el FS borre ese archivo y deje los bloques libres

			resultadoEjecucion=1;
		}

	}

	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void cerrarArchivoFS(int socket_aceptado){//SIN TERMINAR
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
		}else{
			int tiene_permisoLectura=0;
			const char *permiso_lectura = "r";
			if(string_contains(tablaAVer->tablaArchivoPorProceso[i][0], permiso_lectura)){
				tiene_permisoLectura=1;
			}


			if(tiene_permisoLectura==1){
				int punteroADondeVaALeer=tablaAVer->tablaArchivoPorProceso[i][3]+informacionPunteroARecibir;

				//sends a FS mandandole el puntero y el offset , y que me devuelva 0 si no encontro puntero , y 1
				//si salio todo bien y mando la info leida al CPU

				resultadoEjecucion=1;
			}else{
				resultadoEjecucion=0;//No tiene permiso de lectura para ejecutar esta instruccion
				//Faltaria poner algun codigo de error o algo asi
			}




		}

	}
	send(socket_aceptado,&resultadoEjecucion,sizeof(int),0);
}

void guardarArchivoFS(int socket_aceptado){//SIN TERMINAR
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
		}else{


			int tiene_permisoEscritura=0;
			const char *permiso_escritura = "w";
			if(string_contains(tablaAVer->tablaArchivoPorProceso[i][0], permiso_escritura)){
				tiene_permisoEscritura=1;
			}


			if(tiene_permisoEscritura==1){
				int punteroADondeVaALeer=tablaAVer->tablaArchivoPorProceso[i][3]+informacionPunteroARecibir;

				//sends a FS mandandole el puntero y el offset , y que me devuelva 0 si no encontro puntero , y 1
				//si salio todo bien y mando la info leida al CPU
			}else{
				resultadoEjecucion=0;
				//retornar codigo de error o algo asi
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
	int pid;
	int tamanoDireccion;
	int tamanoFlags;
	void* direccion = malloc(tamanoDireccion);
	int descriptorADevolver;
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
		int j,i,k;
		int encontro=0;
		for(i = 0; i < contadorFilasTablaGlobal; ++i)
		{
		   for(j = 0; j<3 ; j++)
		   {

		      if(strcmp(tablaGlobalArchivos[i][0],direccionAValidar) == 0){
		    	  encontro=1;
		    	  tablaGlobalArchivos[i][1]=tablaGlobalArchivos[i][1]+1;//aumento el open

		    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,i);
		      }
		   }
		}

		if(encontro==0){
	    	  contadorFilasTablaGlobal++;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][0]=direccionAValidar;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][1]=1;//el open
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][2]=contadorFilasTablaGlobal;

	    	  //agregarlo en la tabla del proceso
	    	  descriptorADevolver=agregarATablaPorProcesoYDevolverDescriptor(flags,contadorFilasTablaGlobal);

		}

		int validado=1;
		send(socket_aceptado,&validado,sizeof(int),0);
		send(socket_aceptado,&descriptorADevolver,sizeof(int),0);

	}else{
		int validado=0;
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
					borrarArchivoFS(socket_aceptado);
					break;
				case 'O'://obtener datos
					printf("Obteniendo datos del archivo indicado \n");
					obtenerArchivoFS(socket_aceptado);
					break;
				case 'G'://guardar archivo
					printf("Guardando datos del archivo indicado \n");
					guardarArchivoFS(socket_aceptado);
					break;
				case 'P'://guardar archivo
					printf("Cerrando el archivo indicado \n");
					cerrarArchivoFS(socket_aceptado);
					break;
				case 'M'://guardar archivo
					printf("Moviendo puntero \n");
					moverCursorArchivoFS(socket_aceptado);
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
