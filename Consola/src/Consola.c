/*
 ============================================================================
 Name        : Consola.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "Consola.h"
#include "hiloPrograma.h"
#include <time.h>

void signalSigIntHandler(int signum);

int main(void) {



	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logConsola.txt");
	inicializarListas();
	inicializarSemaforos();
	flagCerrarConsola = 1;
	socketKernel = crear_socket_cliente(ipKernel, puertoKernel);

	int err = pthread_create(&hiloInterfazUsuario, NULL, connectionHandler,NULL);
	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));

	signal(SIGINT, signalSigIntHandler);

	pthread_join(hiloInterfazUsuario, NULL);
	log_info(loggerConPantalla,"La Consola ha finalizado");
	return 0;

}

void signalSigIntHandler(int signum)
{
    if (signum == SIGINT)
    {
    	log_warning(loggerConPantalla,"Finalizando consola \n");
    	cerrarTodo();
    	exit(1); /* TODO: Hacer que se termine el hilo interfaz tambien*/

    }
}

void *connectionHandler() {
	char orden;
	while (flagCerrarConsola) {
		sem_wait(&sem_crearHilo);

		imprimirInterfaz();
		scanf(" %c", &orden);

		switch (orden) {
			case 'I':
				crearHiloPrograma();
				break;
			case 'F':
				finalizarPrograma();
				break;
			case 'C':
				limpiarPantalla();
				break;
			case 'Q':
				cerrarTodo();
				break;
			case '\0':
				break;
			default:
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				sem_post(&sem_crearHilo);
				break;
			}
		orden = '\0';
	}
	log_info(loggerConPantalla,"Hilo Interfaz de Usuario de Consola finalizado");
}

void limpiarPantalla(){
	system("clear");
	sem_post(&sem_crearHilo);
}

void finalizarPrograma(){
	char comandoInterruptHandler = 'X';
	char comandoFinalizarPrograma= 'F';
	int procesoATerminar;
	log_info(loggerConPantalla,"Ingresar el PID del programa a finalizar\n");
	scanf("%d", &procesoATerminar);

		bool verificarPid(t_hiloPrograma* proceso){
			return (proceso->pid == procesoATerminar);
		}

		if (list_any_satisfy(listaHilosProgramas,(void*)verificarPid)){

				send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
				send(socketKernel,&comandoFinalizarPrograma,sizeof(char),0);
				send(socketKernel, (void*) &procesoATerminar, sizeof(int), 0);

			}else	log_error(loggerConPantalla,"\nPID incorrecto\n");

		sem_post(&sem_crearHilo);
}

void cerrarTodo(){
	char comandoInterruptHandler='X';
	char comandoCierreConsola = 'E';
	int i;
	int desplazamiento = 0;
	pthread_mutex_lock(&mutexListaHilos);
	int cantidad= listaHilosProgramas->elements_count;

	int mensajeSize = sizeof(int) + sizeof(int)* cantidad;

	char* mensaje= malloc(mensajeSize);

	t_hiloPrograma* procesoACerrar = malloc(sizeof(t_hiloPrograma));

	memcpy(mensaje+desplazamiento,&cantidad,sizeof(int));
	desplazamiento += sizeof(int);

	for(i=0;i<cantidad;i++){
		procesoACerrar = (t_hiloPrograma*) list_get(listaHilosProgramas,i);
		memcpy(mensaje+desplazamiento,&procesoACerrar->pid,sizeof(int));
		desplazamiento += sizeof(int);
	}
	pthread_mutex_unlock(&mutexListaHilos);

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoCierreConsola,sizeof(char),0);
	send(socketKernel,&mensajeSize,sizeof(int),0);
	send(socketKernel,mensaje,mensajeSize,0);


	//recv(socketKernel,&desplazamiento,sizeof(int),0); /*A modo de OK del Kernel*/


	list_destroy_and_destroy_elements(listaHilosProgramas,free);

	free(mensaje);
	flagCerrarConsola = 0;
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

void imprimirInterfaz(){
	printf("----------------------------------------------------------------------\n");
	printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
	printf("----------------------------------------------------------------------\n");
}

void inicializarLog(char *rutaDeLog){
		mkdir("/home/utnso/Log",0755);
		loggerSinPantalla = log_create(rutaDeLog,"Consola", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Consola", true, LOG_LEVEL_INFO);
}

void inicializarListas(){
	listaHilosProgramas= list_create();
}

void inicializarSemaforos(){
	pthread_mutex_init(&mutex_crearHilo,NULL);
	pthread_mutex_init(&mutexListaHilos,NULL);
	pthread_mutex_init(&mutexRecibirDatos,NULL);
	sem_init(&sem_crearHilo,0,1);
}
