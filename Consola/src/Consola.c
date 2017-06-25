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

	int err = pthread_create(&hiloInterfazUsuario, NULL, (void*)connectionHandler,NULL);
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
    	pthread_kill(hiloInterfazUsuario,0);
    	exit(1);

    }
}

void connectionHandler() {
	char orden;
	while (flagCerrarConsola) {
		pthread_mutex_lock(&mutex_crearHilo);

		imprimirInterfaz();
		scanf("%c", &orden);

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
			default:
				log_error(loggerConPantalla,"Orden %c no definida", orden);
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			}
	}
	pthread_kill(hiloInterfazUsuario,0);
}

void limpiarPantalla(){
	system("clear");
	pthread_mutex_unlock(&mutex_crearHilo);
}



void cerrarTodo(){
	char comandoInterruptHandler='X';
	char comandoCierreConsola = 'E';
	int i;
	int desplazamiento = 0;
	pthread_mutex_lock(&mutexListaHilos);
	int cantidad= listaHilosProgramas->elements_count;
	flagCerrarConsola = 0;
	if(cantidad == 0) {
		char comandoCerrarSocket= 'Z';
		send(socketKernel,&comandoCerrarSocket,sizeof(char),0);
		close(socketKernel);
		return;
	}

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


	recv(socketKernel,&desplazamiento,sizeof(int),0); /*A modo de OK del Kernel*/


	list_destroy_and_destroy_elements(listaHilosProgramas,free);

	free(mensaje);
}
void recibirDatosDelKernel(int socketHiloKernel){
	int pid;
	int size;
	int flagCerrarHilo=1;
	char* mensaje;

	recv(socketHiloKernel, &pid, sizeof(int), 0);
	log_info(loggerConPantalla,"Al Programa ANSISOP en socket: %d se le ha asignado el PID: %d", socketHiloKernel,pid);

	cargarHiloPrograma(pid,socketHiloKernel);
	pthread_mutex_unlock(&mutex_crearHilo);

	while(flagCerrarHilo){
		recv(socketHiloKernel,&size,sizeof(int),0);

		pthread_mutex_lock(&mutexRecibirDatos);

		log_warning(loggerConPantalla,"Socket %d recibiendo mensaje para PID %d",socketHiloKernel,pid);

		mensaje = malloc(size * sizeof(char));
		recv(socketHiloKernel,mensaje,size,0);
		strcpy(mensaje+size,"\0");

		if(strcmp(mensaje,"Finalizar")==0) {
			flagCerrarHilo = 0;
			free(mensaje);
			pthread_mutex_unlock(&mutexRecibirDatos);
			break;
		}
		printf("\n%s\n",mensaje);
		actualizarCantidadImpresiones(pid);
		free(mensaje);
		pthread_mutex_unlock(&mutexRecibirDatos);
	}

	gestionarCierrePrograma(pid);
	log_warning(loggerConPantalla,"Hilo Programa ANSISOP--->PID:%d--->Socket:%d ha finalizado",pid,socketHiloKernel);
}

void actualizarCantidadImpresiones(int pid){
	bool verificaPid(t_hiloPrograma* proceso){
			return (proceso->pid == pid);
		}
	pthread_mutex_lock(&mutexListaHilos);
	t_hiloPrograma* programa = list_remove_by_condition(listaHilosProgramas,(void*) verificaPid);
	programa->cantImpresiones += 1;
	list_add(listaHilosProgramas,programa);
	pthread_mutex_unlock(&mutexListaHilos);
}
char* remove_all_chars(char* str, char c) {
	char* str2 = str;
    char *pr = str2, *pw = str2;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
    return str2;
}
int tiempoEjecucion(char* tiempoInicio2,char* tiempoFinalizacion){

	char* tiempoInicioI=remove_all_chars(tiempoInicio2,':');

	char* tiempoFinalizacion2= remove_all_chars(tiempoFinalizacion,':');


	int tiempoInicio3;
	int tiempoFinalizacion3;
	tiempoInicio3= atoi(tiempoInicioI);
	tiempoFinalizacion3= atoi(tiempoFinalizacion2);
	int hora1= tiempoFinalizacion3/100000;
	printf("%d\n",hora1);
	int hora2= tiempoInicio3/100000;
	int hora3 = hora1-hora2;
	printf("%d\n",hora3);
	int tiempoEjecucion = tiempoFinalizacion3-tiempoInicio3;
	tiempoEjecucion = tiempoEjecucion/1000;
	if(hora3==1){
			return (tiempoEjecucion-40);
		}else
	return tiempoEjecucion;

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
