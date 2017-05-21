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


int main(void) {


	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logConsola.txt");

	listaPid = list_create();
	listaHilos= list_create();
	int socketKernel = crear_socket_cliente(ipKernel, puertoKernel);
	int err = pthread_create(&HiloId, NULL, connectionHandler,	(void*) socketKernel);

	if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));

	pthread_join(HiloId, NULL);

	return 0;

}

void *connectionHandler(int socketKernel) {

	while (1) {
		char orden;
		char *ruta = (char*) malloc(200 * sizeof(char));;
		int pidAEliminar=0;
		int tamanoLista=0,i=0;
		t_hilos * hiloACerrar = malloc(sizeof(t_hilos));
		printf("----------------------------------------------------------------------\n");
		printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n");
		printf("----------------------------------------------------------------------\n");
		scanf(" %c", &orden);
		send(socketKernel, (void*) &orden, sizeof(char), 0);


		switch (orden) {
		case 'I':

			printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
			scanf("%s", ruta);
			if ((enviarLecturaArchivo(ruta, socketKernel)) < 0) {

				log_warning(loggerConPantalla,"\nEl archivo indicado es inexistente\n");
			}

			free(ruta);
			break;

		case 'F':

			printf("Ingresar el PID del programa a finalizar\n");
			scanf("%d", &pidAEliminar);


			_Bool verificarPid(Pid* pidNuevoo){
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

				Pid* estructuraPidAEliminar=list_get(listaNueva, 0);
				char *fechaActual = malloc(sizeof(char));
				fechaActual= temporal_get_string_time();

				//int tiempoEjecucion= obtenerTiempoEjecucion(estructuraPidAEliminar->fechaInicio,fechaActual);
				//aca falta la resta de fechas e informarlas
				printf("----------------------------------------------------------------------\n");
				printf("Hora de inicializacion : %s \n Hora de finalizacion: %s\nTiempo de ejecucion: \nCantidad de impresiones: %i\n",estructuraPidAEliminar->fechaInicio,fechaActual,estructuraPidAEliminar->cantImpresiones);
				printf("----------------------------------------------------------------------\n");

				pthread_join(estructuraPidAEliminar->idAsociado, NULL);
				log_info(loggerSinPantalla,"El hilo ha finalizado con exito");
				free(fechaActual);


			}else{
				log_info(loggerConPantalla,"\nPID incorrecto\n");
			}
			break;

		case 'C':
			system("clear");
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

			exit(1);
			break;
		default:
			log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
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


int enviarLecturaArchivo(void *rut, int socket) {
	FILE *f;
	void *mensaje;
	void *bufferArchivo;
	int tamanioArchivo;
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
	log_info(loggerConPantalla,"\nEl mensaje ha sido enviado al kernel\n");

	recibirDatosDelKernelYcrearPrograma(socket);

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
void recibirDatosDelKernelYcrearPrograma (int socketKernel){
		int pid=0;
		int socketEnKernel;
		pthread_t hiloId;
		Pid* pidNuevo = malloc(sizeof(Pid));

		char *tiempoInicio = malloc(sizeof(char));
		tiempoInicio= temporal_get_string_time();


		recv(socketKernel, &pid, sizeof(int), 0);
		recv(socketKernel, &socketEnKernel, sizeof(int),0);
		log_info(loggerConPantalla,"\nEl socket asignado en kernel para el proceso iniciado es: %d \n", socketEnKernel);
		log_info(loggerConPantalla,"\nEl PID asignado es: %d \n", pid);



		int err = pthread_create( &hiloId , NULL , imprimir , &socketEnKernel);
		if (err != 0) log_error(loggerConPantalla,"\nError al crear el hilo :[%s]", strerror(err));
		cargarPid(pidNuevo,pid,tiempoInicio,hiloId);
		cargarHiloId(hiloId);
		list_add(listaPid, pidNuevo);
		list_add(listaHilos,hiloId);


}


void obtenerTiempoEjecucion(char *fechaInicio,char fechaActual){

}


void cargarPid(Pid* pidEstructura, int pid,char* fechaActual,pthread_t hiloId) {
	pidEstructura->cantImpresiones=0;
	pidEstructura->pid = pid;
	pidEstructura->fechaInicio=fechaActual;
	pidEstructura->idAsociado=hiloId;

}
void cargarHiloId(pthread_t hiloId){
	t_hilos* hiloNuevo = malloc(sizeof(t_hilos));
	hiloNuevo->idHilo=hiloId;

}

