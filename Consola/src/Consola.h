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
#include <commons/log.h>
#include <time.h>
#include <signal.h>

//--------LOG----------------//
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//--------LOG----------------//


//--------Configuraciones--------------//
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void imprimirInterfaz();
//--------Configuraciones--------------//


void inicializarListas();

//funciones
//int obtenerTiempoEjecucion(char *fechaInicio,char fechaActual);
void* connectionHandler();
void* imprimir(int socket);
void cargarHiloId(pthread_t hiloId);
void cerrarTodo();
t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;
int socketKernel;

pthread_t hiloInterfazUsuario;


t_list * listaPid;
t_list * listaHilos;
struct tm *tlocal;
static volatile int keepRunning = 1;
