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
#include <arpa/inet.h>


//#define RUTA_LOG "/home/utnso/memoria.log"
t_config* configuracion_memoria;
char* puertoMemoria;
char* ipMemoria;
int marcos;
int marco_size;
int entradas_cache;
int cache_x_proc;
int retardo_memoria;

//Revisar y discutir estructuras

typedef struct
{
	int pid;
	int num_pag;
	int frame;
}struct_adm_memoria;

char* bitMap;

//--------------------Funciones Conexiones----------------------------//
int crear_socket_servidor(char *ip, char *puerto);
int recibirConexion(int socket_servidor);
char* recibir_string(int socket_aceptado);
void enviar_string(int socket, char * mensaje);
void* recibir(int socket);
void enviar(int socket, void* cosaAEnviar, int tamanio);
//----------------------Funciones Conexiones----------------------------//

char nuevaOrdenDeAccion(int puertoCliente);

void *get_in_addr(struct sockaddr *sa);

void leerConfiguracion(char* ruta);
void inicializarMemoriaAdm();//Falta codificar

int main_inicializarPrograma();//Falta codificar
int main_solicitarBytesPagina();//Falta codificar
int main_almacenarBytesPagina();//Falta codificar
int main_asignarPaginaAProceso();//Falta codificar
int main_finalizarPrograma();//Falta codificar

//-----------------------FUNCIONES MEMORIA--------------------------//
int inicializarPrograma(int pid, int cantPaginas);//Falta codificar
int solicitarBytesPagina(int pid,int pagina, int offset, int size);//Falta codificar
int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer);//Falta codificar
int asignarPaginaAProceso(int pid, int cantPaginas);//Falta codificar
int finalizarPrograma(int pid);//Falta codificar
//------------------------------------------------------------------//

void liberarBitMap(int pos, int size);
void ocuparBitMap(int pos, int size);
int verificarEspacio(int size);
void asignarPaginasAProceso(int pid,int posicionFrame,int cantPaginas);

int main(void)
{
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Memoria/config_Memoria");

	bitMap = string_repeat('0',marcos);

	char *frame_Memoria= malloc(marco_size*marcos);

	inicializarMemoriaAdm();

	printf("IP=%s\nPuerto=%s\n",ipMemoria,puertoMemoria);
	int socket_servidor = crear_socket_servidor(ipMemoria,puertoMemoria);
	int socket_cliente = recibirConexion(socket_servidor);

	while(1)
	{
		switch(nuevaOrdenDeAccion(socket_cliente))
		{
		case 'I':
			main_inicializarPrograma();
			break;
		case 'S':
			main_solicitarBytesPagina();
			break;
		case 'A':
			main_almacenarBytesPagina();
			break;
		case 'G':
			main_asignarPaginaAProceso();
			break;
		case 'F':
			main_finalizarPrograma();
			break;
		default:
			printf("Error\n");
			break;
		}
	}
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
	printf("El tamaño a reservar es de %i\n",sizeMemoriaAdm);
	ocuparBitMap(0,sizeMemoriaAdm);

}

int inicializarPrograma(int pid, int cantPaginas)
{
	printf("Inicializar Programa\n");
	int posicionFrame = verificarEspacio(cantPaginas);
	if(posicionFrame >= 0)
	{
		ocuparBitMap(posicionFrame,cantPaginas);
		asignarPaginasAProceso(pid,posicionFrame,cantPaginas);
	}

	return posicionFrame;
}

int solicitarBytesPagina(int pid,int pagina, int offset, int size)
{
	printf("Solicitar Bytes Pagina\n");
	return EXIT_SUCCESS;
}

int almacenarBytesPagina(int pid,int pagina, int offset,int size, char* buffer)
{
	printf("Almacenar Bytes Pagina\n");
	return EXIT_SUCCESS;
}

int asignarPaginaAProceso(int pid, int cantPaginas)
{
	printf("Asignar Pagina A Proceso\n");
	return EXIT_SUCCESS;
}

int finalizarPrograma(int pid)
{
	printf("Finalizar Programa\n");
	return EXIT_SUCCESS;
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
        close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    if (bind(descriptorArchivo, n->ai_addr, n->ai_addrlen) == -1){
    	perror("Error bindeando el socket");
        close(descriptorArchivo);
        freeaddrinfo(infoServer);

        return -1;
    }

    freeaddrinfo(infoServer);

    return descriptorArchivo;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 fd_set master;
	 fd_set read_fds;
	 int fdmax;
	 int i,j;
	 int newfd;
	 socklen_t addr_size;
	 char remoteIP[INET6_ADDRSTRLEN];
	 char buf[256];    // buffer for client data
	 int nbytes;

	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		printf("Error al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		printf("Se puso el socket en listen\n");
	}

	FD_SET(socket_servidor,&master);

	fdmax=socket_servidor;

	for(;;) {
	        read_fds = master; // copy it
	        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	            close(socket_servidor);
	        	perror("select");
	            return 1;
	        }
	 /////////////////////////////////////////////////////////////////////
	 // run through the existing connections looking for data to read
	  for(i = 0; i <= fdmax; i++) {
	       if (FD_ISSET(i, &read_fds)) { // we got one!!
	                if (i == socket_servidor) {
	                           // handle new connections
	                          addr_size = sizeof their_addr;
	                           newfd = accept(socket_servidor,(struct sockaddr *)&their_addr,&addr_size);
	                           	   	 if (newfd == -1) {
	                           	   		 	 perror("accept");
	                           	   	 	 } else {
	                           	   	 		 FD_SET(newfd, &master); // add to master set
	                           	   	 		 	 if (newfd > fdmax) {    // keep track of the max
	                           	   	 		 		 fdmax = newfd;
	                           	   	 		 	 }
	               printf("selectserver: new connection from %s on ""socket %d\n",
	                                       inet_ntop(their_addr.ss_family,
	                                       get_in_addr((struct sockaddr*)&their_addr),
	                                       remoteIP, INET6_ADDRSTRLEN),
	                                       newfd);
	                                             }
	                       	   	   	   	   	   } else {
	                           // handle data from a client
	                           if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
	                               // got error or connection closed by client
	                               if (nbytes == 0) {
	                                   // connection closed
	                                   printf("selectserver: socket %d hung up\n", i);
	                               } else {
	                                   perror("recv");
	                               }
	                               close(i); // bye!
	                               FD_CLR(i, &master); // remove from master set
	                           } else {
	                               // we got some data from a client
	                               //for(j = 0; j <= fdmax; j++) {
	                                   // send to everyone!
	                                   if (FD_ISSET(j, &master)) {
	                                       // except the listener and ourselves
	                                       if (j != socket_servidor && j != i) {
	                                           if (send(j, buf, nbytes, 0) == -1) {
	                                        	perror("send");
	                                           }
	                                       }
	                                   }
	                               //}
	                           }
	                       } // END handle data from client
	                   } // END got new incoming connection
	               } // END looping through file descriptors
	           } // END for(;;)--and you thought it would never end!

	           return 0;
	       }


			///////////////////////////

/*
	addr_size = sizeof(their_addr);
	int socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size);

	if (socket_aceptado == -1){
		close(socket_servidor);
		printf("Error al aceptar conexion\n");
		return 1;
	}



	return socket_aceptado;
	*/

char* recibir_string(int socket_aceptado){
	return (char*) recibir(socket_aceptado);
}

void enviar_string(int socket, char* mensaje){
	int tamanio = string_length(mensaje) + 1;

	enviar(socket, (void*) mensaje, tamanio);
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

char nuevaOrdenDeAccion(int puertoCliente)
{
	char *buffer;
	printf("Esperando Orden del Cliente\n");
	buffer = recibir(puertoCliente);
	printf("Orden %c\n",*buffer);
	return *buffer;
}

int main_inicializarPrograma()
{
	return 0;
}
int main_solicitarBytesPagina()
{
	return 0;
}
int main_almacenarBytesPagina()
{
	return 0;
}
int main_asignarPaginaAProceso()
{
	return 0;
}
int main_finalizarPrograma()
{
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
	int pos = 0, i = 0, espacioLibre = 0;
	while((espacioLibre < size) && (i < marcos))
	{
		if(bitMap[i] == '0')
		{
			if(pos != i)
			{
				espacioLibre ++;
			}
			else
			{
				pos =i;
				espacioLibre ++;
			}
		}
		else
		{
			espacioLibre = 0;
			pos = i +1;
		}
		i++;
	}
	if(espacioLibre == size)
	{
		return pos;
	}
	else
	{
		return -1;
	}
}

void asignarPaginasAProceso(int pid,int posicionFrame,int cantPaginas)
{

}
