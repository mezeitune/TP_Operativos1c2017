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

char* bitMap;
void* frame_Memoria; //Corregir movimiento de punteros

int socket_servidor;

//the thread function
void *connection_handler(void *);
void connection_Listener(int socket_desc);

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
char* solicitarBytesPagina(int pid,int pagina, int offset, int size);
int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer);
int asignarPaginasAProceso(int pid, int cantPaginas, int frame);
int finalizarPrograma(int pid);
//------------------------------------------------------------------//

void liberarBitMap(int pos, int size);
void ocuparBitMap(int pos, int size);
int verificarEspacio(int size);
void escribirEstructuraAdmAMemoria(int pid, int frame, int cantPaginas, int cantPaginasAnteriores);
void borrarProgramDeStructAdms(int pid);
int buscarFrameDePaginaDeProceso(int pid, int pagina);

void imprimirBitMap();
void imprimirEstructurasAdministrativas();
int buscarFrameVacio();

int main(void)
{
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Memoria/config_Memoria");
	imprimirConfiguraciones();

	bitMap = string_repeat('0',marcos);

	frame_Memoria= malloc(marco_size*marcos);

	inicializarMemoriaAdm();

	imprimirBitMap();
	imprimirEstructurasAdministrativas();

	socket_servidor = crear_socket_servidor(ipMemoria,puertoMemoria);
	int socket_Kernel = recibirConexion(socket_servidor);

	connection_Listener(socket_Kernel);

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
	int sizeMemoriaAdm = ((sizeof(int)*3*marcos)+marco_size-1)/marco_size;
	printf("Las estructuras administrativas ocupan %i paginas\n",sizeMemoriaAdm);
	printf("---------------------------------------------------\n");
	ocuparBitMap(0,sizeMemoriaAdm);
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	aux.pid=-1;
	aux.num_pag=-1;
	aux.frame = i;
	while(i < marcos)
	{
		if(i<sizeMemoriaAdm)
		{
			aux.pid = -9;
			aux.num_pag=i;
		}
		else
		{
			aux.pid = -1;
			aux.num_pag=-1;
		}
		memcpy(frame_Memoria + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
		i++;
		aux.frame = i;
	}
}

int inicializarPrograma(int pid, int cantPaginas)
{
	printf("Inicializar Programa %d\n",pid);
	int posicionFrame = verificarEspacio(cantPaginas);
	if(posicionFrame >= 0)
	{
		ocuparBitMap(posicionFrame,cantPaginas);
		asignarPaginasAProceso(pid,cantPaginas,posicionFrame);
	}

	return posicionFrame;
}

char* solicitarBytesPagina(int pid,int pagina, int offset, int size)
{
	printf("Solicitar Bytes Pagina %d del proceso %d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	printf("El frame es : %d\n",frame);

	char* buffer=malloc(size);
	memcpy(buffer,frame_Memoria + frame*marco_size+offset,size);

	printf("%s\n",buffer);


	return buffer;
}

int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer)
{
	printf("Almacenar Bytes A Pagina:%d del proceso:%d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	printf("Frame:%d\n",frame);
	if(frame >= 0)
	{
		memcpy(frame_Memoria + frame*marco_size+offset,buffer,size);
	}
	else
	{
		printf("No se encontró el PID/Pagina del programa\n");
	}
	return EXIT_SUCCESS;
}

int asignarPaginasAProceso(int pid, int cantPaginas, int posicionFrame)
{
	int cantPaginasAnteriores = cantPaginasDeProceso(pid);
	escribirEstructuraAdmAMemoria(pid,posicionFrame,cantPaginas,cantPaginasAnteriores);
	return EXIT_SUCCESS;
}

int finalizarPrograma(int pid)
{
	printf("Finalizar Programa:%d\n",pid);
	borrarProgramDeStructAdms(pid);

	return EXIT_SUCCESS;
}

int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 socklen_t addr_size;


	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		printf("Error al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		printf("Se puso el socket en listen\n");
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
		printf("Error al aceptar conexion\n");
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
	printf("PID:%d\n",pid);
	recv(sock,&cantPaginas,sizeof(int),0);
	printf("CantPaginas:%d\n",cantPaginas);
	int espacioLibre = verificarEspacio(cantPaginas);
	printf("Bitmap:%s\n",bitMap);
	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
			i++;
		}
		return 0;
	}
	else
	{
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
	printf("PID:%d\n",pid);
	recv(sock,&pagina,sizeof(int),0);
	printf("Pagina:%d\n",pagina);
	recv(sock,&offset,sizeof(int),0);
	printf("Offset:%d\n",offset);
	recv(sock,&size,sizeof(int),0);
	printf("Size:%d\n",size);

	bufferAEnviar= solicitarBytesPagina(pid,pagina,offset,size);

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
	printf("PID:%d\n",pid);
	recv(sock,&pagina,sizeof(int),0);
	printf("Pagina:%d\n",pagina);
	recv(sock,&offset,sizeof(int),0);
	printf("Offset:%d\n",offset);
	recv(sock,&size,sizeof(int),0);
	printf("Size:%d\n",size);
	bytes=malloc(size);
	recv(sock,bytes,size,MSG_WAITALL);
	printf("%s\n",bytes);
	almacenarBytesPagina(pid,pagina,offset,size,bytes);
	printf("Sali de almacenar \n");
	free(bytes);
	return 0;
}
int main_asignarPaginasAProceso(int sock)
{
	int pid;
	int cantPaginas;
	recv(sock,&pid,sizeof(int),0);
	printf("PID:%d\n",pid);
	recv(sock,&cantPaginas,sizeof(int),0);
	printf("CantPaginas:%d\n",cantPaginas);
	int espacioLibre = verificarEspacio(cantPaginas);
	printf("Bitmap:%s\n",bitMap);
	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
		}
		return 0;
	}
	else
	{
		printf("No hay espacio suficiente en la memoria\n");
		return -1;
	}
}
int main_finalizarPrograma(int sock)
{
	int pid;
	pid=atoi((char*)recibir(sock));
	finalizarPrograma(pid);
	return 0;
}

void liberarBitMap(int pos, int size)
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

int verificarEspacio(int size)
{
	int i = 0, espacioLibre = 0;
	while(i < marcos)
	{
		if(bitMap[i] == '0')
		{
		espacioLibre ++;
		i++;
		}
		i++;
	}
	return espacioLibre;
}


/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char orden;
    int resultadoDeEjecucion;
    char *buffer;
	while((orden=nuevaOrdenDeAccion(sock)) != 'Q')
	{
		switch(orden)
		{
		case 'A':
			resultadoDeEjecucion = main_inicializarPrograma(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'S':
			resultadoDeEjecucion = main_solicitarBytesPagina(sock);
			break;
		case 'C':
			resultadoDeEjecucion = main_almacenarBytesPagina(sock);
			break;
		case 'G':
			resultadoDeEjecucion = main_asignarPaginasAProceso(sock);
			break;
		case 'F':
			resultadoDeEjecucion = main_finalizarPrograma(sock);
			break;
		case 'Q':
			printf("Cliente %d desconectado",sock);
			fflush(stdout);
			break;
		case 'X':
			perror("recv failed");
			break;
		default:
			printf("Error: Orden %c no definida\n",orden);
			break;
		}
		printf("Resultado de ejecucion:%d\n",resultadoDeEjecucion);
		imprimirBitMap();
		imprimirEstructurasAdministrativas();
	}
    return 0;
}

void connection_Listener(int socket_desc)
{
	//Atiendo al socket del kernel
	if( pthread_create( &thread_id , NULL , connection_handler , (void*) &socket_desc) < 0)
	{
		perror("could not create thread");
	}
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
		memcpy(frame_Memoria + frame*desplazamiento + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
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
		memcpy(&aux, frame_Memoria + i*desplazamiento,sizeof(struct_adm_memoria)); //Revisar si esto funciona
		if(aux.pid == pid) //Si el PID del programa en mi estructura Aadministrativa es igual al del programa que quiero borrar
		{
			liberarBitMap(aux.frame,1); //Marco la posición del frame que me ocupa esa página en particular de ese programa como vacía
			aux.num_pag = -1;
			aux.pid = -1;
			memcpy(frame_Memoria + i*desplazamiento,&aux,sizeof(struct_adm_memoria)); //Marco que en ese frame no tengo un programa asignado
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
		memcpy(&aux, frame_Memoria + i*desplazamiento,sizeof(struct_adm_memoria));
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

void imprimirBitMap()
{
	printf("BitMap:%s\n",bitMap);
}

void imprimirEstructurasAdministrativas()
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	printf("Frame/PID/NumPag\n");
	while(i < marcos)
	{
		memcpy(&aux, frame_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		i++;
		printf("/___%d/__%d/__%d\n",aux.frame,aux.pid,aux.num_pag);
	}
}

int buscarFrameVacio()
{
	int i = -1;
	while(i < marcos)
	{
		if(bitMap[i] == '0')
		{
			return i;
		}
		i++;
	}
	return i;
}

int cantPaginasDeProceso(int pid)
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	int cantPaginas = 0;
	while(i < marcos)
	{
		memcpy(&aux, frame_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			cantPaginas ++;
		}
		i++;
	}
	return cantPaginas;
}
