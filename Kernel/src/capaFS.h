#ifndef _CAPAFS_
#define _CAPAFS_
#include "conexiones.h"



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

		    		  descriptorADevolver=3;

		    	  }else{
		    		  //tabla existe
		    		  t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
		    		  tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);
		    		  tablaAVer->contadorFilasTablaPorProceso++;
		    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][0]=flags;
		    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][1]=tablaGlobalArchivos[i][3];//apunta al indice de la global
		    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2]=tablaAVer->contadorFilasTablaPorProceso+3;//FileDescriptor siempre empieza en 3

		    		  descriptorADevolver=tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2];
		    	  }


		      }
		   }
		}





		if(encontro==0){
	    	  contadorFilasTablaGlobal++;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][0]=direccionAValidar;
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][1]=1;//el open
	    	  tablaGlobalArchivos[contadorFilasTablaGlobal][2]=contadorFilasTablaGlobal;

	    	  //agregarlo en la tabla del proceso

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
	    		  tablaAAgregar->contadorFilasTablaPorProceso;
	    		  list_add(listaTablasArchivosPorProceso,tablaAAgregar);
	    		  int ultimoElemento=list_size(listaTablasArchivosPorProceso)-1;
	    		  t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
	    		  tablaAVer=list_get(listaTablasArchivosPorProceso,ultimoElemento);

	    		  tablaAVer->tablaArchivoPorProceso[0][0]=flags;
	    		  tablaAVer->tablaArchivoPorProceso[0][1]=tablaGlobalArchivos[i][3];//apunta al indice de la global
	    		  tablaAVer->tablaArchivoPorProceso[0][2]=3;//FileDescriptor siempre empieza en 3

	    		  descriptorADevolver=3;

	    	  }else{
	    		  //tabla existe
	    		  t_tablaArchivoPorProceso* tablaAVer = malloc(sizeof(t_tablaArchivoPorProceso));
	    		  tablaAVer=list_get(listaTablasArchivosPorProceso,dondeEstaElPid);
	    		  tablaAVer->contadorFilasTablaPorProceso++;
	    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][0]=flags;
	    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][1]=tablaGlobalArchivos[i][3];//apunta al indice de la global
	    		  tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2]=tablaAVer->contadorFilasTablaPorProceso+3;//FileDescriptor siempre empieza en 3

	    		  descriptorADevolver=tablaAVer->tablaArchivoPorProceso[tablaAVer->contadorFilasTablaPorProceso][2];
	    	  }

		}





		//si existe le aumento el Open y lo abro en su tabla con su pid correspondiente y sus flags


		int validado=1;
		send(socket_aceptado,&descriptorADevolver,sizeof(int),0);
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
