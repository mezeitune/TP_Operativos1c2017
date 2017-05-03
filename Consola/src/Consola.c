/*
 ============================================================================
 Name        : Consola.c
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




int enviarLecturaArchivo(void *ruta, int socket);
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void* connectionHandler(int socket);
//void inicializarPID(int pid);
//void enviarPIDAEliminar(int pidAFinalizar,int socket);

t_config* configuracion_Consola;
char* ipKernel;
char* puertoKernel;


pthread_t HiloId;



typedef struct tm *tlocal;

typedef struct {
	int pid;
	tlocal tmIniciado;
	int cantImpresiones;
} Pid ;

void cargarPid(Pid* pidEstructura, int pid, tlocal *tlocal) {
	pidEstructura->pid = pid;
	pidEstructura->tmIniciado=*tlocal;
	pidEstructura->cantImpresiones=0;
			//printf("now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

}
t_list * listaPid;


int main(void) {


	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logConsola.txt");
	listaPid = list_create();

	int socketKernel = crear_socket_cliente(ipKernel, puertoKernel);

	int err = pthread_create(&HiloId, NULL, connectionHandler,	(void*) socketKernel);

	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));

	else printf("\n Thread created successfully\n");

	(void) pthread_join(HiloId, NULL);

	return EXIT_SUCCESS;

}

void *connectionHandler(int socket) {

	while (1) {
		char orden;
		char *ruta = (char*) malloc(200 * sizeof(char));
		;


		int pidAEliminar=0;


		printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
		scanf(" %c", &orden);
		send(socket, (void*) &orden, sizeof(char), 0);

		switch (orden) {
		case 'I':
			printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
			scanf("%s", ruta);
			if ((enviarLecturaArchivo(ruta, socket)) < 0) {

				log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente\n");
			}

			break;
		case 'F':

			printf("Ingresar el PID del programa a finalizar\n");
			scanf("%d", &pidAEliminar);


			_Bool verificarPid(Pid* pidNuevoo){

				return (pidNuevoo->pid== pidAEliminar);

			}


			t_list * listaNueva;
			listaNueva= list_create();
			listaNueva= list_filter(listaPid,verificarPid);

			int estaVacia =  list_size(listaNueva);
			if (estaVacia==1){
				tlocal tiempo = time(0);
			    struct tm *tlocal = localtime(&tiempo);
			    char output[128];
				strftime(output,128,"Fecha y hora Fin de ejecucion : %d/%m/%y %H:%M:%S",tlocal);

				Pid* pidAEliminar=list_get(listaNueva, 0);
				//tlocal TiempoTotalEjecucion = tiempo - pidAEliminar->tmIniciado;
				/*typedef struct{
    int dia;
    int mes;
    int anio;
}Fecha;

int regularBisiesto( int tFecha );
int numeroBisiestos( int anio );
int diaDelAnio( Fecha fecha );

// Algoritmo para calcular el número de días entre dos fechas
// Los cálculos realizados son:
// Si las fechas son del mismo año se calcula el intervalo:
//   - fecha1 -> fecha2
// Si las fechas son de diferentes años se calculan 3 intervalos:
//   - fecha1 -> fin año fecha1
//   - días por los años intermedios
//   - inicio año fecha2 -> fecha2
int main(void){
  Fecha fecha1 = { 1,1, 2050 }, fecha2 = { 12,12, 2050 };
  int difDias = 0;

  // Cálculo del intervalo fecha1 -> fecha2
  // no se tienen en cuenta los años en este punto porque la corrección se realiza después
  difDias = diaDelAnio( fecha2 ) - diaDelAnio( fecha1 );

  if( fecha1.anio != fecha2.anio )
  {
    // Días por los años completos
    difDias += (fecha2.anio - fecha1.anio - 1) * 365;
    difDias += numeroBisiestos(fecha2.anio - 1) - numeroBisiestos(fecha1.anio);

    // Cálculo correspondiente al intervalo fecha1 -> fin año fecha1
    fecha1.dia = 31;
    fecha1.mes = 12;
    difDias += diaDelAnio( fecha1 );
  }

  printf("\n Del %d/%d/%d al %d/%d/%d hay %d dias", fecha1.dia, fecha1.mes, fecha1.anio,
            fecha2.dia, fecha2.mes, fecha2.anio, difDias);

  getchar();
  return 0;
}

int regularBisiesto( int anio)
{
  return ((anio%4 == 0 && anio%100 != 0) || (anio%400) == 0);
}

int numeroBisiestos( int anio )
{
  return anio / 4 - anio / 100 + anio / 400;
}

int diaDelAnio( Fecha fecha )
{
  int tot_dias[] = { 00,31,28,31,30,31,30,31,31,30,31,30,31 };

  int dias = fecha.dia;
  int mes;

  for( mes = 1; mes < fecha.mes; ++mes )
    dias += tot_dias[mes];

  if( fecha.mes > 2 )
    dias += regularBisiesto( fecha.anio );

  return dias;
}*/



				printf("%s\nCantidad de impresiones %i",output,pidAEliminar->cantImpresiones);

				list_remove_by_condition(listaPid, verificarPid);
				send(socket, (void*) &pidAEliminar, sizeof(int), 0);

				log_info(loggerConPantalla,"\nEl programa AnSISOP de PID : %d  ha finalizado\n",pidAEliminar);

			}else{
				printf("PID incorrecto\n");
			}
			break;
		case 'C':
			system("clear");
			break;
		case 'Q':

			list_destroy_and_destroy_elements(listaPid, free);
			log_warning(loggerConPantalla,"\nSe ha desconectado el cliente\n");
			exit(1);
			break;
		default:
			log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
			break;
		}

	}
}

int enviarLecturaArchivo(void *rut, int socket) {
	FILE *f;
	void *mensaje;
	void *bufferArchivo;
	int tamanioArchivo;
	int pid=0;
	char *ruta = (char *) rut;

	/* TODO Validar el nombre del archivo */

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);

	bufferArchivo = malloc(tamanioArchivo); // Pido memoria para leer el contenido del archivo

	if (bufferArchivo == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(bufferArchivo);
		exit(2);
	}

	mensaje = malloc(sizeof(int) * 2 + tamanioArchivo); // Pido memoria para el mensaje EMPAQUETADO que voy a mandar

	if (mensaje == NULL) {

		log_error(loggerConPantalla,"\nNo se pudo conseguir memoria\n");
		free(mensaje);
		free(bufferArchivo);
		exit(2);
	}

	fread(bufferArchivo, sizeof(bufferArchivo), tamanioArchivo, f);

	memcpy(mensaje, &tamanioArchivo, sizeof(int)); // Empaqueto en el mensaje el tamano del archivo a enviar.
	memcpy(mensaje + sizeof(int), bufferArchivo, tamanioArchivo); // Empaqueto en el mensjae, el contenido del archivo.

	send(socket, mensaje, tamanioArchivo + sizeof(int), 0); // Mando el mensjae empaquetado.
	printf("El mensaje ha sido enviado \n");

	recv(socket, &pid, sizeof(int), 0);

	log_info(loggerConPantalla,"\nEl socket asignado para el proceso iniciado es: %d \n", pid);
	log_info(loggerConPantalla,"\nEl PID asignado es: %d \n", pid);

	//creo lista con el pid

	Pid* pidNuevo = malloc(sizeof(pid));
	time_t tiempo = time(0);
	cargarPid(pidNuevo, pid,localtime(&tiempo) );
	list_add(listaPid, pidNuevo);
	//lista con los pid

	free(bufferArchivo);
	free(mensaje);
	return 0;

}


void leerConfiguracion(char* ruta) {
	configuracion_Consola = config_create(ruta);
	ipKernel = config_get_string_value(configuracion_Consola, "IP_KERNEL");
	puertoKernel = config_get_string_value(configuracion_Consola,"PUERTO_KERNEL");
}

void imprimirConfiguraciones() {

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\n", ipKernel,
			puertoKernel);
	printf("---------------------------------------------------\n");
}


void inicializarLog(char *rutaDeLog){


		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Consola", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Consola", true, LOG_LEVEL_INFO);

}



