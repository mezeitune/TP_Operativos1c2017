#include "CPU.h"


int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");
	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	//send(socketKernel,'N',sizeof(char),0);


	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);


	log_info(loggerConPantalla, "Inicia proceso CPU");
	recibirTamanioPagina(socketKernel);
	listaPcb = list_create();
	signal(SIGUSR1, signalSigusrHandler);


	if(cpuOcupada == 1){
			recibirPCB();
			cpuOcupada--;
	}else
		log_warning(loggerConPantalla, "No se le asigno un PCB a esta CPU");

	return 0;
}

void recibirPCB(){


		char comandoRecibirPCB;
		char comandoGetNuevoProceso = 'N';
		send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);
		recv(socketKernel,&comandoRecibirPCB,sizeof(char),0);
		log_info(loggerConPantalla, "Se ha avisado que se quiere enviar un PCB...\n");
		connectionHandlerKernel(socketKernel,comandoRecibirPCB);

}

void connectionHandlerKernel(int socketAceptado, char orden) {

	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	switch (orden) {
		case 'S':
			log_info(loggerConPantalla, "Se esta por asignar un PCB");
				establecerPCB(socketAceptado);
					break;
		default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
	}
orden = '\0';
return;
}

void establecerPCB(){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb = recibirYDeserializarPcb(socketKernel);

	log_info(loggerConPantalla, "CPU recibe PCB correctamente\n");


	list_add(listaPcb, pcb);

	ciclosDeQuantum(pcb);
	interfazHandler(pcb);//esto despues se saca, es para probar funciones nomas

}
void ciclosDeQuantum(t_pcb* pcb){
	int i = 0;
	int quantum_definido=4;
	//recv(socketKernel,&quantum_definido,sizeof(int),0);
	while((i < quantum_definido)){
		ejecutarInstruccion(pcb);
		i++;
	}
	//if((i == quantum_definido)){
		//send(socketKernel,&comandoTerminoElQuantum , sizeof(char),0);
	//}
}
void ejecutarInstruccion(t_pcb* pcb){

	char* instruccion = obtener_instruccion(pcb);
	printf("Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);//que haga lo que tenga q hacer
	free(instruccion);
	pcb->programCounter = pcb->programCounter + 1;

}

int conseguirDatosMemoria (char** instruccion, t_pcb* pcb, int paginaSolicitada,int offset,int size){
	int resultadoEjecucion;
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina
	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*instruccion = recibir_string(socketMemoria);
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

int almacenarDatosEnMemoria(t_pcb* pcb,char* buffer, int size){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';

		int paginaDondeGuardoDatos = 1; // valor arbitrario

		send(socketMemoria,&comandoAlmacenar,sizeof(char),0);
		send(socketMemoria,&pcb->pid,sizeof(int),0);
		send(socketMemoria,&paginaDondeGuardoDatos,sizeof(int),0);
		//send(socketMemoria,&pcb->offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);

		printf("Se almaceno correctamente %s",buffer);
		return resultadoEjecucion;

}

void imprimirPCB(t_pcb * pcb){
	printf("Pid: %d\n", pcb->pid);
	printf("Program Counter: %d\n", pcb->programCounter);

	printf("Cantidad de Instrucciones: %d\n", pcb->cantidadInstrucciones);
	printf("Cantidad de paginas en memoria: %d\n", pcb->cantidadPaginasCodigo);
	printf("\n-------Indice de Codigo-------\n");
	int i;
	for(i = 0; i < pcb->cantidadInstrucciones; i++){
		printf("Instruccion %d:  \t%d %d\n", i, pcb->indiceCodigo[i][0], pcb->indiceCodigo[i][1]);
	}

	printf("\n-------Indice de Etiquetas-------\n");
	printf("%s\n", pcb->indiceEtiquetas);

}

void interfazHandler(t_pcb* pcb){

	char orden;
	int size = strlen("bonebone");
	char* instruccion;

	while(1){
		while(orden != 'Q'){

			printf("\nIngresar orden:\n");
			scanf(" %c", &orden);

			switch(orden){
				case 'S':
					imprimirPCB(pcb);
					//instruccion = conseguirDatosMemoria(pcb,0,6,16);//el 0 es la pagina a la cual buscar y el size es de la instruccion

					break;

				case 'F':
					finalizar();

					log_info(loggerConPantalla,"\nSea finalizado el programa con pid%s\n" , pcb->pid);
					free(pcb);
					break;
				case 'Q':
					log_warning(loggerConPantalla,"\nSe ha desconectado el cliente\n");
					free(pcb);
					exit(1);
					break;
				default:
					log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
					break;
			}
		}
			orden = '\0';
	}
}


void finalizar (){

	//comando para  avisarle al kernel que debe eliminar
	char comandoInicializacion = 'F';

	void* pcbAEliminar= malloc(sizeof(char) + sizeof(int) * 2); //pido memoria para el comando que deba usar el kernel + los 2 int de la estructura del pcb

	//agarro el pcb que quiero eliminar
	t_pcb *unPcbAEliminar = list_get(listaPcb,0);

	//pido memoria para ese pcb
	t_pcb *infoPcbAEliminar = malloc(sizeof(t_pcb));

	//asigno pcb en memoria
	infoPcbAEliminar->pid = unPcbAEliminar->pid;
	//infoPcbAEliminar->cantidadPaginas = unPcbAEliminar->cantidadPaginas;

	//printf("%d%d",unPcbAEliminar->pid,unPcbAEliminar->cantidadPaginas);
	//serializarPCByEnviar(socketKernel,comandoInicializacion,&infoPcbAEliminar,pcbAEliminar);

	list_destroy_and_destroy_elements(listaPcb, free);


	//un send avisandole al kernel q termino ese proceso con su header
	free(pcbAEliminar);

}

char* obtener_instruccion(t_pcb * pcb){
	int program_counter = pcb->programCounter;
	int byte_inicio_instruccion = pcb->indiceCodigo[program_counter][0];
	int bytes_tamanio_instruccion = pcb->indiceCodigo[program_counter][1];

	int num_pagina = byte_inicio_instruccion / paginaSize;
	int offset = byte_inicio_instruccion - (num_pagina * paginaSize);//no es 0 porque evita el begin
	char* mensajeRecibido;
	char* instruccion;
	char* continuacion_instruccion;
	int bytes_a_leer_primera_pagina;

	if (bytes_tamanio_instruccion > (paginaSize * 2)){
		printf("El tamanio de la instruccion es mayor al tamanio de pagina\n");
	}
	if ((offset + bytes_tamanio_instruccion) < paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_tamanio_instruccion)<0)
			printf("No se pudo solicitar el contenido\n");
		else
			instruccion=mensajeRecibido;
	} else {
		bytes_a_leer_primera_pagina = paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					printf("No se pudo solicitar el contenido\n");
		else
					instruccion=mensajeRecibido;

		log_info(loggerConPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido,pcb, (num_pagina + 1),offset,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
								printf("No se pudo solicitar el contenido\n");
					else
								continuacion_instruccion=mensajeRecibido;
			log_info(loggerConPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(loggerConPantalla, "La continuacion de la instruccion es 0. Ni la leo");
		}
	}
	char** string_cortado = string_split(instruccion, "\n");
	free(instruccion);
	instruccion = string_new();
	string_append(&instruccion, string_cortado[0]);
	log_info(loggerConPantalla, "\nInstruccion obtenida: %s", instruccion);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return instruccion;
}


void signalSigusrHandler(int signum)
{
    if (signum == SIGUSR1)
    {
        //printf("Received SIGUSR1!\n");

    	//hacer un send a memoria para avisar que se desconecto la CPU y que no se ponga como loca
		log_warning(loggerConPantalla,"\nSe ha desconectado CPU con signal SIGUSR1\n");
		exit(1);
    }
}
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(loggerConPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(loggerConPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
}

void leerConfiguracion(char* ruta) {
	configuracion_memoria = config_create(ruta);
	ipMemoria = config_get_string_value(configuracion_memoria, "IP_MEMORIA");
	puertoKernel= config_get_string_value(configuracion_memoria, "PUERTO_KERNEL");
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO_MEMORIA");
	ipKernel = config_get_string_value(configuracion_memoria,"IP_KERNEL");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nPUERTO KERNEL:%s\nIP KERNEL:%s\nPUERTO MEMORIA:%s\nIP MEMORIA:%s\n",puertoKernel,ipKernel,puertoMemoria,ipMemoria);
	printf("---------------------------------------------------\n");
}
void inicializarLog(char *rutaDeLog){

	mkdir("/home/utnso/Log",0755);

	loggerSinPantalla = log_create(rutaDeLog,"CPU", false, LOG_LEVEL_INFO);
	loggerConPantalla = log_create(rutaDeLog,"CPU", true, LOG_LEVEL_INFO);
}

