/*
 ============================================================================
 Name        : Memoria.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conexiones.h"
#include <commons/log.h>



//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//



char* puertoMemoria;//4000
char* ipMemoria;
int marcos;
int marco_size;
int entradas_cache;
int cache_x_proc;
int retardo_memoria;
int contadorConexiones=0;
pthread_t thread_id;
t_config* configuracion_memoria;

typedef struct
{
	int frame;
	int pid;
	int num_pag;
}struct_adm_memoria;

typedef struct
{
	int entrada;
	int pid;
	int num_pag;
}struct_adm_cache;

//char* bitMap;
void* bloque_Memoria;
void* bloque_Cache;

int socket_servidor;

//the thread function
void *connection_handler(void *);
void *connection_Listener();

//--------------------Funciones Conexiones----------------------------//
int recibirConexion(int socket_servidor);
//----------------------Funciones Conexiones----------------------------//

char nuevaOrdenDeAccion(int puertoCliente);

void imprimirConfiguraciones();
void leerConfiguracion(char* ruta);
void inicializarMemoriaAdm();

int main_inicializarPrograma();
int main_solicitarBytesPagina();
int main_almacenarBytesPagina();
int main_asignarPaginasAProceso();
int main_finalizarPrograma();

//-----------------------FUNCIONES MEMORIA--------------------------//
int inicializarPrograma(int pid, int cantPaginas);
char* solicitarBytesPagina(int pid,int pagina, int offset, int size,char** buffer);
int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer);
int asignarPaginasAProceso(int pid, int cantPaginas, int frame);
int finalizarPrograma(int pid);
//------------------------------------------------------------------//

//void liberarBitMap(int pos, int size);
//void ocuparBitMap(int pos, int size);
int verificarEspacioLibre();
void escribirEstructuraAdmAMemoria(int pid, int frame, int cantPaginas, int cantPaginasAnteriores);
void borrarProgramDeStructAdms(int pid);
int buscarFrameDePaginaDeProceso(int pid, int pagina);

//void imprimirBitMap();
void imprimirEstructurasAdministrativas();
int buscarFrameVacio();

void interfazHandler();
void modificarRetardo();
void dumpDeMemoria();
void flush();
void size();

void dumpCache();
void contenidoDeMemoria();
void datosAlmacenadosDeProceso();
void datosAlmacenadosEnMemoria();
void vaciarCache();
void tamanioProceso();
void tamanioMemoria();

void inicializarCache();
int buscarFrameDePaginaDeProcesoEnCache(int pid, int pagina);

int main(void)
{
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Memoria/config_Memoria");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logMemoria.txt");

	//bitMap = string_repeat('0',marcos);

	bloque_Memoria= malloc(marco_size*marcos);
	bloque_Cache = malloc(entradas_cache*marco_size);

	inicializarMemoriaAdm();
	inicializarCache();

	//imprimirBitMap();
	imprimirEstructurasAdministrativas();

	socket_servidor = crear_socket_servidor(ipMemoria,puertoMemoria);

	pthread_create( &thread_id , NULL , connection_Listener,NULL);

	interfazHandler();

	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta)
{
	configuracion_memoria = config_create(ruta);
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO");
	ipMemoria = config_get_string_value(configuracion_memoria,"IP_MEMORIA");
	marcos = config_get_int_value(configuracion_memoria,"MARCOS");
	marco_size = config_get_int_value(configuracion_memoria,"MARCO_SIZE");
	entradas_cache = config_get_int_value(configuracion_memoria,"ENTRADAS_CACHE");
	cache_x_proc = config_get_int_value(configuracion_memoria,"CACHE_X_PROC");
	retardo_memoria = config_get_int_value(configuracion_memoria,"RETARDO_MEMORIA");
}

void inicializarMemoriaAdm()
{
	int sizeStructsAdmMemoria = ((sizeof(struct_adm_memoria)*marcos)+marco_size-1)/marco_size;

	log_info(loggerConPantalla,"Las estructuras administrativas de la memoria ocupan %i frames\n",sizeStructsAdmMemoria);

	printf("---------------------------------------------------\n");

	//ocuparBitMap(0,sizeMemoriaAdm);
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	aux.pid=-1;
	aux.num_pag=-1;
	aux.frame = i;
	while(i < marcos)
	{
		if(i<sizeStructsAdmMemoria)
		{
			aux.pid = -9;
			aux.num_pag=i;
		}
		else
		{
			aux.pid = -1;
			aux.num_pag=-1;
		}
		memcpy(bloque_Memoria + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
		i++;
		aux.frame = i;
	}
}

int inicializarPrograma(int pid, int cantPaginas)
{
	log_info(loggerConPantalla,"Inicializar Programa %d\n",pid);
	int posicionFrame = verificarEspacioLibre();
	if(posicionFrame >= 0)
	{
		//ocuparBitMap(posicionFrame,cantPaginas);
		asignarPaginasAProceso(pid,cantPaginas,posicionFrame);
	}

	return posicionFrame;
}

char* solicitarBytesPagina(int pid,int pagina, int offset, int size, char** buffer)
{
	printf("Solicitar Bytes Pagina %d del proceso %d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	printf("El frame es : %d\n",frame);

	memcpy(*buffer,bloque_Memoria + frame*marco_size+offset,size);

	printf("%s\n",*buffer);

	return *buffer;
}

int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer)
{
	printf("Almacenar Bytes A Pagina:%d del proceso:%d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	printf("Frame:%d\n",frame);
	if(frame >= 0)
	{
		memcpy(bloque_Memoria + frame*marco_size+offset,buffer,size);
	}
	else
	{
		log_warning(loggerConPantalla,"No se encontró el PID/Pagina del programa\n");
	}
	return EXIT_SUCCESS;
}

int asignarPaginasAProceso(int pid, int cantPaginas, int posicionFrame)
{
	escribirEstructuraAdmAMemoria(pid,posicionFrame,cantPaginas,cantPaginasDeProceso(pid));
	return EXIT_SUCCESS;
}

int finalizarPrograma(int pid)
{
	log_info(loggerConPantalla,"\nFinalizar Programa:%d\n",pid);
	borrarProgramDeStructAdms(pid);

	return EXIT_SUCCESS;
}

int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 socklen_t addr_size;


	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		log_info(loggerConPantalla,"\nError al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		log_info(loggerConPantalla,"\nSe puso el socket en listen\n");
		printf("---------------------------------------------------\n");
	}

	addr_size = sizeof(their_addr);

	int socket_aceptado;
    socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size);

	contadorConexiones ++;
	printf("\n----------Nueva Conexion aceptada numero: %d ---------\n",contadorConexiones);
	printf("----------Handler asignado a (%d) ---------\n",contadorConexiones);

	if (socket_aceptado == -1){
		close(socket_servidor);
		log_error(loggerConPantalla,"\nError al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}

char nuevaOrdenDeAccion(int socketCliente)
{
	char* buffer=malloc(sizeof(char));
	char bufferRecibido;

	printf("\n--Esperando una orden del cliente-- \n");

	recv(socketCliente,buffer,sizeof(char),0);
	bufferRecibido = *buffer;

	free(buffer);

    printf("El cliente ha enviado la orden: %c\n",bufferRecibido);
	printf("%c\n",bufferRecibido);
	return bufferRecibido;
}

int main_inicializarPrograma(int sock)
{
	int pid;
	int cantPaginas;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&cantPaginas,sizeof(int),0);

	printf("PID:%d\n",pid);
	printf("CantPaginas:%d\n",cantPaginas);

	int espacioLibre = verificarEspacioLibre();

	//printf("Bitmap:%s\n",bitMap);

	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			//ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
			i++;
		}
		sleep(retardo_memoria);
		return 0;
	}
	else
	{
		sleep(retardo_memoria);
		printf("No hay espacio suficiente en la memoria\n");
		return -1;
	}
}
int main_solicitarBytesPagina(int sock)
{
	int pid;
	int pagina;
	int offset;
	int size;
	char*bufferAEnviar;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&pagina,sizeof(int),0);
	recv(sock,&offset,sizeof(int),0);
	recv(sock,&size,sizeof(int),0);

	printf("PID:%d\n",pid);
	printf("Pagina:%d\n",pagina);
	printf("Offset:%d\n",offset);
	printf("Size:%d\n",size);

	bufferAEnviar=malloc(size);
	solicitarBytesPagina(pid,pagina,offset,size,&bufferAEnviar);
	sleep(retardo_memoria);
	enviar_string(sock,bufferAEnviar);
	//send(sock,bufferAEnviar,size,0);

	free(bufferAEnviar);
	return 0;
}
int main_almacenarBytesPagina(int sock)
{
	int pid;
	int pagina;
	int offset;
	int size;
	char *bytes;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&pagina,sizeof(int),0);
	recv(sock,&offset,sizeof(int),0);
	recv(sock,&size,sizeof(int),0);
	bytes=malloc(size);
	recv(sock,bytes,size,MSG_WAITALL);

	printf("PID:%d\n",pid);
	printf("Pagina:%d\n",pagina);
	printf("Offset:%d\n",offset);
	printf("Size:%d\n",size);

	almacenarBytesPagina(pid,pagina,offset,size,bytes);
	free(bytes);
	sleep(retardo_memoria);
	return 0;
}
int main_asignarPaginasAProceso(int sock)
{
	int pid;
	int cantPaginas;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&cantPaginas,sizeof(int),0);

	printf("PID:%d\n",pid);
	printf("CantPaginas:%d\n",cantPaginas);
	//printf("Bitmap:%s\n",bitMap);

	int espacioLibre = verificarEspacioLibre();

	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			//ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
			i++;
		}
		sleep(retardo_memoria);
		return 0;
	}
	else
	{
		sleep(retardo_memoria);
		printf("No hay espacio suficiente en la memoria\n");
		return -1;
	}
}
int main_finalizarPrograma(int sock)
{
	int pid;

	recv(sock,&pid,sizeof(int),0);

	finalizarPrograma(pid);
	sleep(retardo_memoria);
	return 0;
}

/*void liberarBitMap(int pos, int size)
{
	int i;
	for (i=0; i< size; i++)
	{
		bitMap[pos+i] = (char) '0';
	}
}

void ocuparBitMap(int pos, int size)
{
	int i;
	for (i=0; i< size; i++)
	{
		bitMap[pos+i] = (char) '1';
	}
}
*/

int verificarEspacioLibre()
{
	struct_adm_memoria aux;
	int i = 0, espacioLibre = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == -1)
		{
			espacioLibre++;
		}
		i++;
	}

	return espacioLibre;
}


/*
 * Funcion que atiende las conexiones de los clientes
 * */
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char orden;
    int resultadoDeEjecucion;
	while((orden=nuevaOrdenDeAccion(sock)) != '\0');
	{
		switch(orden)
		{
		case 'A':
			resultadoDeEjecucion = main_inicializarPrograma(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'S':
			resultadoDeEjecucion = main_solicitarBytesPagina(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'C':
			resultadoDeEjecucion = main_almacenarBytesPagina(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'G':
			resultadoDeEjecucion = main_asignarPaginasAProceso(sock);
			break;
		case 'F':
			resultadoDeEjecucion = main_finalizarPrograma(sock);
			break;
		case 'X':
			perror("recv failed");
			break;
		default:
			log_warning(loggerConPantalla,"\nError: Orden %c no definida\n",orden);
			break;
		}
		printf("Resultado de ejecucion:%d\n",resultadoDeEjecucion);
		//imprimirBitMap();
		imprimirEstructurasAdministrativas();
	}
	shutdown(sock,SHUT_RDWR);
	printf("Cliente %d desconectado",sock);
    return 0;
}

void* connection_Listener()
{
	int sock;
	while(1)
	{
		//Quedo a la espera de CPUs y las atiendo
		sock = recibirConexion(socket_servidor);
		if( pthread_create( &thread_id , NULL , connection_handler , (void*) &sock) < 0)
		{
			perror("could not create thread");
		}
	}
}

void escribirEstructuraAdmAMemoria(int pid, int frame, int cantPaginas, int cantPaginasAnteriores)
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	aux.pid=pid;
	aux.num_pag=cantPaginasAnteriores;
	aux.frame = frame;
	while(i < cantPaginas)
	{
		memcpy(bloque_Memoria + frame*desplazamiento + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
		aux.num_pag++;
		aux.frame++;
		i++;
	}
}

void borrarProgramDeStructAdms(int pid)
{
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	struct_adm_memoria aux;
	while(i<marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento,sizeof(struct_adm_memoria)); //Revisar si esto funciona
		if(aux.pid == pid) //Si el PID del programa en mi estructura Aadministrativa es igual al del programa que quiero borrar
		{
			//liberarBitMap(aux.frame,1); //Marco la posición del frame que me ocupa esa página en particular de ese programa como vacía
			aux.num_pag = -1;
			aux.pid = -1;
			memcpy(bloque_Memoria + i*desplazamiento,&aux,sizeof(struct_adm_memoria)); //Marco que en ese frame no tengo un programa asignado
		}
		i++;
	}
}

int buscarFrameDePaginaDeProceso(int pid, int pagina)
{
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	struct_adm_memoria aux;
	while(i<marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento,sizeof(struct_adm_memoria));
		if(aux.pid == pid && aux.num_pag == pagina) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero borrar
		{
			return aux.frame;
		}
		i++;
	}
	return -1;
}

void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP:%s\nPUERTO:%s\nMARCOS:%d\nTAMAÑO MARCO:%d\nENTRADAS CACHE:%d\nCACHE POR PROCESOS:%d\nRETARDO MEMORIA:%d\n",ipMemoria,puertoMemoria,marcos,marco_size,entradas_cache,cache_x_proc,retardo_memoria);
		printf("---------------------------------------------------\n");
}

/*void imprimirBitMap()
{
	printf("BitMap:%s\n",bitMap);
}
*/

void imprimirEstructurasAdministrativas()
{
	struct_adm_memoria auxMemoria;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	printf("---Estructuras Adm De la Memoria---\n");
	printf("Frame/PID/NumPag\n");
	while(i < marcos)
	{
		memcpy(&auxMemoria, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		i++;
		printf("/___%d/__%d/__%d\n",auxMemoria.frame,auxMemoria.pid,auxMemoria.num_pag);
	}



	struct_adm_cache auxCache;
	i = 0;
	desplazamiento = sizeof(struct_adm_cache);
	printf("---Estructuras Adm De la Cache---\n");
	printf("Entrada/PID/NumPag\n");
	while(i < entradas_cache)
	{
		memcpy(&auxCache, bloque_Cache + i*desplazamiento, sizeof(struct_adm_cache));
		i++;
		printf("/___%d/__%d/__%d\n",auxCache.entrada,auxCache.pid,auxCache.num_pag);
	}
	printf("---------------------------------------------------\n");
}

int buscarFrameVacio()
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == -1)
		{
			return aux.frame;
		}
		i++;
	}
	return -1;
}

int cantPaginasDeProceso(int pid)
{

	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	int cantPaginas = 0;
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			cantPaginas ++;
		}
		i++;
	}
	return cantPaginas;
}

void inicializarLog(char *rutaDeLog)
{
	mkdir("/home/utnso/Log",0755);

	loggerSinPantalla = log_create(rutaDeLog,"Memoria", false, LOG_LEVEL_INFO);
	loggerConPantalla = log_create(rutaDeLog,"Memoria", true, LOG_LEVEL_INFO);
}

void interfazHandler()
{
	char orden;
	printf("Esperando una orden para la interfaz\nR-Modificar Retardo\nD-Dump\nF-Flush\nS-Size\n");
	scanf("%c",&orden);
	while(orden != 'Q')
	{
		switch(orden)
		{
			case 'R':
			{
				modificarRetardo();
				break;
			}
			case 'D':
			{
				dumpDeMemoria();
				break;
			}
			case 'F':
			{
				flush();
				break;
			}
			case 'S':
			{
				size();
				break;
			}
			default:
			{
				log_warning(loggerConPantalla,"Error: Orden de interfaz no reconocida, ingrese una orden nuevamente\n");
				break;
			}
		}
		printf("Esperando una orden para la interfaz\nR-Modificar Retardo\nD-Dump\nF-Flush\nS-Size\n");
		scanf("%c",&orden);
	}
}

void modificarRetardo()
{
	printf("Ingrese el nuevo retardo de la memoria\n");
	scanf("%d",&retardo_memoria);
	printf("El retardo de la memoria a sido cambiado a: %d\n",retardo_memoria);
}

void dumpDeMemoria()
{
	printf("Elija qué sección de la memoria desea imprimir\n");
	printf("C-Dump De Cache\nE-Dump de estructuras administrativas\nM-Contenido en Memoria\n");
	char orden;
	scanf("%c",&orden);
	switch(orden)
	{
		case 'C':
		{
			dumpCache();
			break;
		}
		case 'E':
		{
			imprimirEstructurasAdministrativas();
			break;
		}
		case 'M':
		{
			contenidoDeMemoria();
			break;
		}
		default:
		{
			log_warning(loggerConPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void flush()
{
	printf("--Flush--\n");
	vaciarCache();
}

void size()
{
	char orden;
	scanf("%c",&orden);
	switch(orden)
	{
		case 'P':
		{
			tamanioProceso();
			break;
		}
		case 'M':
		{
			tamanioMemoria();
			break;
		}
		default:
		{
			log_warning(loggerConPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void dumpCache()
{
	printf("--Dump De Cache--\n");
}

void contenidoDeMemoria()
{
	printf("--Contenido De Memoria--\n");
	printf("T-Mostrar datos almacenados de todos los procesos\nU-Datos almacenados de todos los procesos\n");
	char orden;
	scanf("%c",&orden);
	switch(orden)
	{
		case 'T':
		{
			datosAlmacenadosEnMemoria();
			break;
		}
		case 'U':
		{
			datosAlmacenadosDeProceso();
			break;
		}
		default:
		{
			log_warning(loggerConPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void datosAlmacenadosDeProceso()
{
	printf("--Datos Almacenados De Proceso--\n");
	printf("Ingrese el PID del proceso:\n");
	int pid;
	scanf("%d",&pid);
	printf("PID:%d\n",pid);
	struct_adm_memoria aux;
	int i = 0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);
	char* datosFrame = malloc(marco_size);
	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			memcpy(&datosFrame, bloque_Memoria + aux.frame*marco_size, marco_size);
			printf("%s\n",datosFrame);
		}
		i++;
	}
	free(datosFrame);
}

void datosAlmacenadosEnMemoria()
{
	printf("--Datos Almacenados En Memoria--\n");

	struct_adm_memoria aux;
	int i = 0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);
	char* datosFrame = malloc(marco_size);
	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid != -9 && aux.pid != -1)
		{
			printf("PID:%d\nFrame:%d\n",aux.pid,aux.frame);
			memcpy(&datosFrame, bloque_Memoria + aux.frame*marco_size, marco_size);
			printf("Datos:%s\n",datosFrame);
		}
		i++;
	}
	free(datosFrame);
}

void vaciarCache()
{
	printf("--Vaciar Cache--\n");
}

void tamanioProceso()
{
	printf("--Tamaño Proceso--\n");
	printf("Ingrese el PID del proceso:\n");
	int pid;
	scanf("%d",&pid);
	printf("PID:%d\n",pid);
	struct_adm_memoria aux;
	int i = 0,espacioTotal =0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);

	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			espacioTotal++;
		}
		i++;
	}
	printf("El proceso %d ocupa %d paginas\n",pid,espacioTotal);
}

void tamanioMemoria()
{
	int espacioLibre = verificarEspacioLibre();
	int espacioOcupado = marcos - espacioLibre;
	printf("Frames Totales:%d\nEspacios Libres:%d\nEspacios Ocupados:%d\n",marcos,espacioLibre,espacioOcupado);
}

void inicializarCache()
{
	int sizeStructsAdmCache= ((sizeof(struct_adm_cache)*entradas_cache)+marco_size-1)/marco_size;
	log_info(loggerConPantalla,"Las estructuras administrativas de la cache ocupan %i entradas\n",sizeStructsAdmCache);


	printf("-------------Inicializar Cache-------------\n");

	//ocuparBitMap(0,sizeMemoriaAdm);
	struct_adm_cache aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_cache);
	aux.pid=-1;
	aux.num_pag=-1;
	aux.entrada = i;
	while(i < entradas_cache)
	{
		if(i<sizeStructsAdmCache)
		{
			aux.pid = -9;
			aux.num_pag=i;
		}
		else
		{
			aux.pid = -1;
			aux.num_pag=-1;
		}
		memcpy(bloque_Cache + i*desplazamiento, &aux, sizeof(struct_adm_cache));
		i++;
		aux.entrada = i;
	}
}

int buscarFrameDePaginaDeProcesoEnCache(int pid, int pagina)
{
	int i = 0;
	int desplazamiento = sizeof(struct_adm_cache);
	struct_adm_cache aux;
	while(i<entradas_cache)
	{
		memcpy(&aux, bloque_Cache + i*desplazamiento,sizeof(struct_adm_cache));
		if(aux.pid == pid && aux.num_pag == pagina) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero borrar
		{
			return aux.entrada;
		}
		i++;
	}
	return -1;
}









