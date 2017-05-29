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

	pthread_join(hiloInterfazUsuario, NULL);

	return 0;

}

void *connectionHandler() {

	while (1) {
		char orden;
		int pidAEliminar=0;
		int tamanoLista=0,i=0;
		t_hilos * hiloACerrar = malloc(sizeof(t_hilos));

		pthread_mutex_lock(&mutex_crearHilo);

		imprimirInterfaz();
		scanf(" %c", &orden);

		switch (orden) {
			case 'I':
				crearHiloPrograma();
				break;
			case 'F': /*TODO: Emprolijar esto*/

				printf("Ingresar el PID del programa a finalizar\n");
				scanf("%d", &pidAEliminar);


				_Bool verificarPid(t_hiloPrograma* pidNuevoo){
					return (pidNuevoo->pid == pidAEliminar);
					}

				t_list * listaNueva;
				listaNueva= list_create();
				listaNueva= list_filter(listaPid,verificarPid);

				int estaVacia =  list_size(listaNueva);

				if (estaVacia==1){

					list_remove_by_condition(listaPid, verificarPid);
					send(socketKernel, (void*) &pidAEliminar, sizeof(int), 0);
					printf("----------------------------------------------------------------------\n");
					log_info(loggerConPantalla,"\nEl programa AnSISOP de PID : %d  ha finalizado",pidAEliminar);

					t_hiloPrograma* estructuraPidAEliminar=list_get(listaNueva, 0);
					char *fechaActual = malloc(sizeof(char));
					fechaActual= temporal_get_string_time();

					//int tiempoEjecucion= obtenerTiempoEjecucion(estructuraPidAEliminar->fechaInicio,fechaActual);
					//aca falta la resta de fechas e informarlas
					printf("----------------------------------------------------------------------\n");
					printf("Hora de inicializacion : %s \n Hora de finalizacion: %s\nTiempo de ejecucion: \nCantidad de impresiones: %i\n",estructuraPidAEliminar->fechaInicio,fechaActual,estructuraPidAEliminar->cantImpresiones);
					printf("----------------------------------------------------------------------\n");

					//pthread_join(estructuraPidAEliminar->idAsociado, NULL);
					log_info(loggerSinPantalla,"El hilo ha finalizado con exito");
					free(fechaActual);


				}else{
					log_info(loggerConPantalla,"\nPID incorrecto\n");
				}
				pthread_mutex_unlock(&mutex_crearHilo);
				break;

			case 'C':
				system("clear");
				pthread_mutex_unlock(&mutex_crearHilo);

				break;

			case 'Q':
				tamanoLista = list_size(listaHilos);

				for(i=0;i<tamanoLista;i++){
						hiloACerrar= list_get(listaHilos,i);
						pthread_join(hiloACerrar->idHilo, NULL);

					}

				list_destroy_and_destroy_elements(listaPid, free);
				list_destroy_and_destroy_elements(listaHilos, free);
				log_warning(loggerConPantalla,"\nSe ha desconectado la consola\n");
				log_info(loggerSinPantalla,"Los hilos se han finalizado con exito");
				free(hiloACerrar);
				pthread_mutex_unlock(&mutex_crearHilo);

				exit(1);
				break;
			default:
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				pthread_mutex_unlock(&mutex_crearHilo);

				break;
			}

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

void inicializarListas(){
	listaPid = list_create();
	listaHilos= list_create();
	listaHilosProgramas= list_create();
}


void obtenerTiempoEjecucion(char *fechaInicio,char fechaActual){

}


void cargarHiloId(pthread_t hiloId){
	t_hilos* hiloNuevo = malloc(sizeof(t_hilos));
	hiloNuevo->idHilo=hiloId;
	list_add(listaHilos,hiloNuevo);
}

void imprimirInterfaz(){
	printf("----------------------------------------------------------------------\n");
	printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
	printf("----------------------------------------------------------------------\n");
}

