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
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/log.h>
#include "conexiones.h"
#include <time.h>

//--------LOG----------------//
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//
 //estructura pid
typedef struct {
	int pid;
	char* fechaInicio;
	int cantImpresiones;
	pthread_t* idAsociado;
} Pid ;
typedef struct {
	pthread_t* idHilo;
} t_hilos;

//funciones
//int obtenerTiempoEjecucion(char *fechaInicio,char fechaActual);
int enviarLecturaArchivo(void *ruta, int socket);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void* connectionHandler(int socket);
void* imprimir(int socket);
void cargarPid (Pid* pidEstructura, int pid,char* fechaActual,pthread_t hiloId);
void cargarHiloId(pthread_t hiloId);
void recibirDatosDelKernelYcrearPrograma(int socketEnKernel);


t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;

pthread_t HiloId;


t_list * listaPid;
t_list * listaHilos;
struct tm *tlocal;
