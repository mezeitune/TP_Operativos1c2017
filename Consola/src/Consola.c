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
void finalizarPrograma();

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logConsola.txt");
	inicializarListas();

	pthread_mutex_init(&mutex_crearHilo,NULL);
	socketKernel = crear_socket_cliente(ipKernel, puertoKernel);
	pthread_mutex_unlock(&mutex_crearHilo);

	int err = pthread_create(&hiloInterfazUsuario, NULL, connectionHandler,NULL);
	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));
	signal(SIGINT, signalSigIntHandler);

	pthread_join(hiloInterfazUsuario, NULL);

	return 0;

}

void signalSigIntHandler(int signum)
{
    if (signum == SIGINT)
    {
    	log_warning(loggerConPantalla,"Cierre por signal, cerrando hilos programa y avisando a kernel \n");
    	cerrarTodo();
    	log_warning(loggerConPantalla,"La consola se ha cerrado por signal correctamente \n");
    	exit(1);
    }
}

void *connectionHandler() {

	while (1) {
		char orden;
		pthread_mutex_lock(&mutex_crearHilo);

		imprimirInterfaz();
		scanf(" %c", &orden);

		switch (orden) {
			case 'I':
				crearHiloPrograma();
				break;
			case 'F':
				finalizarPrograma(); /*TODO: Verificar que se liberan bien los recursos*//*TODO: Verificar que el hilo termine y libere sus recursos*/
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			case 'C':
				system("clear");
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			case 'Q':
				cerrarTodo(); /*TODO: Verificar que los hilos terminen y liberen sus recursos*/
				exit(1);
				break;
			default:
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			}
		orden = '\0';
	}
}
void imprimirHandler(int socket, char *orden) {// Recibe un char* para tener la variable modificada cuando vuelva a selectorConexiones()

	recv(socket,orden,sizeof(int)  ,0);
	//printf( "El nuevo cliente %d ha enviado la orden: %c\n",socketAceptado, *(char*)orden);


	switch (*(char*)orden) {

		case 'I':
				imprimir(socket);

		break;


		default:
				if(*orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", *(char*)orden);
				break;
		} // END switch de la consola

	*orden = '\0';
	return;

}
void *imprimir(int socket){

				int bytesARecibir=0;

				char* buffer;
				recv(socket ,&bytesARecibir, sizeof(int),0); //recibo la cantidad de bytes del mensaje del kernel
				buffer = malloc(bytesARecibir); // Pido memoria para recibir el contenido del mensaje con los bytes que recibi antes
				recv(socket,buffer,bytesARecibir ,0);//recibo el mensaje de la kenrel con el tamaño de bytesArecibir

				printf("%s",buffer);
				return 0;
}

void finalizarPrograma(){
	char comandoInterruptHandler = 'X';
	char comandoFinalizarPrograma= 'F';
	char* mensajeResultado;
	int size;
	int procesoATerminar;
	log_info(loggerConPantalla,"Ingresar el PID del programa a finalizar\n");
	scanf("%d", &procesoATerminar);


		bool verificarPid(t_hiloPrograma* proceso){
			return (proceso->pid == procesoATerminar);
		}

		if (list_any_satisfy(listaHilosProgramas,(void*)verificarPid)){

				t_hiloPrograma* programaAFinalizar = list_remove_by_condition(listaHilosProgramas,(void*)verificarPid);
				send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
				send(socketKernel,&comandoFinalizarPrograma,sizeof(char),0);
				send(socketKernel, (void*) &procesoATerminar, sizeof(int), 0);

				time_t tiempoFinalizacion = time(0);
				struct tm* fechaFinalizacion = localtime(&tiempoFinalizacion);
				double tiempoEjecucion= difftime(tiempoFinalizacion,mktime(programaAFinalizar->fechaInicio));

				printf("----------------------------------------------------------------------\n");
				printf("Hora de inicializacion : %s \n Hora de finalizacion: %s\nTiempo de ejecucion: %e \nCantidad de impresiones: %d\n",asctime(programaAFinalizar->fechaInicio),asctime(fechaFinalizacion),tiempoEjecucion,programaAFinalizar->cantImpresiones);
				printf("----------------------------------------------------------------------\n");


				pthread_detach(programaAFinalizar->idHilo);
				log_info(loggerSinPantalla,"El hilo ha finalizado con exito");


				recv(socketKernel,&size,sizeof(int),0);
				recv(socketKernel,&mensajeResultado,size,0);
				log_info(loggerSinPantalla,mensajeResultado);
				free(mensajeResultado);
				free(programaAFinalizar);
				free(fechaFinalizacion);

			}else{
						log_info(loggerConPantalla,"\nPID incorrecto\n");
			}
}


void cerrarTodo(){
	char comandoInterruptHandler='X';
	char comandoCierreConsola = 'E';
	int i;

	int mensajeSize = sizeof(int)* listaHilosProgramas->elements_count;
	char* mensaje= malloc(mensajeSize);
	char* procesosATerminar = mensaje;
	t_hiloPrograma* procesoACerrar = malloc(sizeof(t_hiloPrograma));

	for(i=0;i<listaHilosProgramas->elements_count;i++){
		procesoACerrar = (t_hiloPrograma*) list_get(listaHilosProgramas,i);
		memcpy(procesosATerminar,&procesoACerrar->pid,sizeof(int));
		procesosATerminar += sizeof(int);
		pthread_detach(procesoACerrar->idHilo);
	}
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoCierreConsola,sizeof(char),0);
	send(socketKernel,&mensajeSize,sizeof(int),0);
	send(socketKernel,mensaje,mensajeSize,0);
	free(procesoACerrar);
	free(mensaje);


			log_info(loggerSinPantalla,"Los hilos se han finalizado con exito");

			list_destroy_and_destroy_elements(listaHilosProgramas,free);
			pthread_mutex_unlock(&mutex_crearHilo);
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
