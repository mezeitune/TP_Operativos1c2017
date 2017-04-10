/*
 ============================================================================
 Name        : Kernel.c
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
#include <pthread.h>

t_config* configuracion_kernel;
pthread_t thread_id;

int crear_socket_cliente(char * ip, char * puerto);
int crear_socket_servidor(char *ip, char *puerto);
void enviar(int socket, void* cosaAEnviar, int tamanio);
int recibirConexion(int socket_servidor);
void leerConfiguracion(char* ruta);
void *connection_handler(void *socket_desc);
char nuevaOrdenDeAccion(int puertoCliente);
void* recibir(int socket);

char *ipMemoria;
char *puertoProg;//2001
char *puertoCPU;//3001
char *puertoMemoria;//4040s
char *ipProg;
char *ipFileSys;
char *puertoFileSys;
char *quantum;
char *quantumSleep;
char *algoritmo;
char *gradoMultiProg;
char *semIds;
char *semInit;
char *sharedVars;
char *ipCPU;
char *stackSize;
pthread_t thread_id;








//the thread function
void *sock_Prog();
//the thread function
void *sock_CPU();
//the thread function
void *sock_FS();
//the thread function
void *sock_Memoria();


int main(void)
{
	//char orden;

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Kernel/config_Kernel");

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP CONSOLA:%s\nPUERTO CONSOLA:%s\nIP CPU:%s\nPUERTO CPU:%s\nIP FS:%s\nPUERTO FS:%s\n",ipMemoria,puertoMemoria,ipProg,puertoProg,ipCPU,puertoCPU,ipFileSys,puertoFileSys);
	printf("---------------------------------------------------\n");
	printf("QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%s\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%s\n",quantum,quantumSleep,algoritmo,gradoMultiProg,semIds,semInit,sharedVars,stackSize);
	printf("---------------------------------------------------\n");

	//int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria); //Variable definidas
	//int socket_servidor = crear_socket_servidor(ipMemoria,puertoFileSys);
	//int socket_servidor1 = crear_socket_servidor(ipMemoria,puertoCPU);
	//recibirConexion(socket_servidor1);



    if( pthread_create( &thread_id , NULL ,  sock_CPU , (void*) NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }
    if( pthread_create( &thread_id , NULL ,  sock_Prog , (void*) NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    if( pthread_create( &thread_id , NULL ,  sock_FS , (void*) NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    if( pthread_create( &thread_id , NULL ,  sock_Memoria , (void*) NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }
	int socket_servidorP = crear_socket_servidor(ipProg,puertoProg);
		recibirConexion(socket_servidorP);



/*	while(1)

	printf("CONFIGURACIONES\nipMemoria=%s\npuertoProg=%s\npuertoCPU=%s\npuertoMemoria=%s\nipFileSys=%s\npuertoFileSys=%s\nquantum=%s\nquantumSleep=%s\nalgoritmo=%s\ngradoMultiProg=%s\nsemIds=%s\nsemInit=%s\nsharedVars=%s\n",ipMemoria,puertoProg,puertoCPU,puertoMemoria,ipFileSys,puertoFileSys,quantum,quantumSleep,algoritmo,gradoMultiProg,semIds,semInit,sharedVars);

	int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria); //Variable definidas
	while(1)

		{
			scanf(" %c", &orden);
			enviar(socket_Memoria,(void*) &orden,sizeof(char));
		}
	return 0;
	*/

		return 0;
}

void* sock_Prog(){
	int socket_servidorProg = crear_socket_servidor(ipProg,puertoProg);
	recibirConexion(socket_servidorProg);
}




void* sock_FS(){
	char orden;
	//int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria);
	int socket_FS = crear_socket_cliente("127.0.0.1","5001");
	while(orden != 'Q')
	{
		scanf(" %c", &orden);
		enviar(socket_FS,(void*) &orden,sizeof(char));
	}
	//int socket_servidorFS = crear_socket_servidor(ipFileSys,puertoFileSys);
	//recibirConexion(socket_servidorFS);
}

void* sock_Memoria(){
	char orden;
	//	int socket_Memoria = crear_socket_cliente(ipMemoria,puertoMemoria);
	int socket_Mem = crear_socket_cliente(ipMemoria,puertoMemoria);
	while(orden != 'Q')
	{
		scanf("%c", &orden);
		enviar(socket_Mem,(void*) &orden,sizeof(char));
	}
}


void* sock_CPU(){
	int socket_servidor = crear_socket_servidor("127.0.0.1",puertoCPU);
	recibirConexion(socket_servidor);
}

int crear_socket_cliente(char * ip, char * puerto){

    int descriptorArchivo, estado;
    struct addrinfo hints, *infoServer, *n;

    memset(&hints,0,sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((estado = getaddrinfo(ip, puerto, &hints, &infoServer)) != 0){
        fprintf(stderr, "Error en getaddrinfo: %s", gai_strerror(estado));
        return -1;
    }

    for(n = infoServer; n != NULL; n = n->ai_next){
        descriptorArchivo = socket(n->ai_family, n->ai_socktype, n->ai_protocol);
        if(descriptorArchivo != -1)
            break;
    }

    if(descriptorArchivo == -1){
        perror("Error al crear el socket");
        freeaddrinfo(infoServer);
        return -1;
    }

    estado = connect(descriptorArchivo, n->ai_addr, n->ai_addrlen);

    if (estado == -1){
        perror("Error conectando el socket");
        freeaddrinfo(infoServer);
        return -1;
    }

    freeaddrinfo(infoServer);

    return descriptorArchivo;
}

int crear_socket_servidor(char *ip, char *puerto){
    int descriptorArchivo, estado;
    struct addrinfo hints, *infoServer, *n;

    memset(&hints,0,sizeof (struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((estado = getaddrinfo(ip, puerto, &hints, &infoServer)) != 0){
        fprintf(stderr, "Error en getaddrinfo: %s", gai_strerror(estado));

        return -1;
    }

    for(n = infoServer; n != NULL; n = n->ai_next){
        descriptorArchivo = socket(n->ai_family, n->ai_socktype, n->ai_protocol);
        if(descriptorArchivo != -1) break;
    }

    if(descriptorArchivo == -1){
        perror("Error al crear el socket");
        freeaddrinfo(infoServer);

        return -1;
    }

    int si = 1;

    if(setsockopt(descriptorArchivo,SOL_SOCKET,SO_REUSEADDR,&si,sizeof(int)) == -1){
    	perror("Error en setsockopt");
     //   close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    if (bind(descriptorArchivo, n->ai_addr, n->ai_addrlen) == -1){
    	perror("Error bindeando el socket");
     //   close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    freeaddrinfo(infoServer);

    return descriptorArchivo;
}
void enviar(int socket, void* cosaAEnviar, int tamanio){
	void* mensaje = malloc(sizeof(int) + tamanio);
	void* aux = mensaje;
	*((int*)aux) = tamanio;
	aux += sizeof(int);
	memcpy(aux, cosaAEnviar, tamanio);

	send(socket, mensaje, sizeof(int) + tamanio, 0);
	free(mensaje);
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
	}

	addr_size = sizeof(their_addr);

	int socket_aceptado;
    while( (socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size)) )
    {
        puts("Connection accepted");

        if(pthread_create( &thread_id , NULL ,  connection_handler , (void*) &socket_aceptado) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }


	if (socket_aceptado == -1){
		close(socket_servidor);
		printf("Error al aceptar conexion\n");
		return 1;
	}

	return socket_aceptado;
}

// Misma funcion que en Memoria. Solo para testear
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char orden = 'F';
	while(orden != 'Q')
	{
		orden = nuevaOrdenDeAccion(sock);
		switch(orden)
		{
		case 'I':
			 printf("/nI");
			//main_inicializarPrograma();
			break;
		case 'S':
			//main_solicitarBytesPagina();
			break;
		case 'A':
			//main_almacenarBytesPagina();
			break;
		case 'G':
			//main_asignarPaginaAProceso();
			break;
		case 'F':
			//main_finalizarPrograma();
			break;
		case 'Q':
			puts("Cliente desconectado");
			fflush(stdout);
			break;
		case 'X':
			perror("recv failed");
			break;
		default:
			printf("Error\n");
			break;
		}
	}
    return 0;
}

char nuevaOrdenDeAccion(int puertoCliente)
{
	char *buffer;
	printf("Esperando Orden del Cliente\n");
	buffer = recibir(puertoCliente);
	//int size_mensaje = sizeof(buffer);
    if(buffer == NULL)
    {
        return 'Q';
    	//puts("Client disconnected");
        //fflush(stdout);
    }
    else if(buffer == -1)
    {
        return 'X';
    	//perror("recv failed");
    }
	printf("Orden %c\n",*buffer);
	return *buffer;
}
void* recibir(int socket){
	int checkSocket = -1;

	void* recibido = malloc(sizeof(int));

	checkSocket = read(socket, recibido, sizeof(int));

	int tamanioDelMensaje = *((int*)recibido);

	free(recibido);

	if(!checkSocket) return NULL;

	recibido = malloc(tamanioDelMensaje);

	int bytesRecibidos = 0;

	while(bytesRecibidos < tamanioDelMensaje && checkSocket){
		checkSocket = read(socket, (recibido + bytesRecibidos), (tamanioDelMensaje - bytesRecibidos));
		bytesRecibidos += checkSocket;
	}

	return !checkSocket ? NULL:recibido;
}

void leerConfiguracion(char* ruta){

	configuracion_kernel = config_create(ruta);
	puertoProg = config_get_string_value(configuracion_kernel,"PUERTO_PROG");
	ipProg = config_get_string_value(configuracion_kernel,"IP_PROG");
	puertoCPU = config_get_string_value(configuracion_kernel,"PUERTO_CPU");
	ipCPU = config_get_string_value(configuracion_kernel,"IP_CPU");
	ipMemoria = config_get_string_value(configuracion_kernel,"IP_MEMORIA");
	puertoMemoria = config_get_string_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel,"IP_FS");
	puertoFileSys = config_get_string_value(configuracion_kernel,"PUERTO_FS");
	quantum = config_get_string_value(configuracion_kernel,"QUANTUM");
	quantumSleep = config_get_string_value(configuracion_kernel,"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion_kernel,"ALGORTIMO");
	gradoMultiProg = config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION");
	semIds = config_get_string_value(configuracion_kernel,"SEM_IDS");
	semInit = config_get_string_value(configuracion_kernel,"SEM_INIT");
	sharedVars = config_get_string_value(configuracion_kernel,"SHARED_VARS");
	stackSize= config_get_string_value(configuracion_kernel,"STACK_SIZE");

}
