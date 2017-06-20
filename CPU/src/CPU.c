#include "PrimitivasDeInstrucciones.h"
#include "PrimitivasKernel.h"

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);

	log_info(loggerConPantalla, "Inicia proceso CPU");

	signal(SIGUSR1, signalHandler);

	recibirTamanioPagina(socketKernel);
	enviarAlKernelPedidoDeNuevoProceso(socketKernel);
	recibirYMostrarAlgortimoDePlanificacion(socketKernel);

	esperarPCB();

	return 0;
}


//----------------------------------Manejo PCB------------------------------------------
void esperarPCB(){

	while(cpuOcupada==1){
		log_info(loggerConPantalla," CPU Esperando un script");
		cantidadInstruccionesAEjecutarPorKernel = quantum;
		recibirPCB();
		cpuOcupada--;
	}

}
void recibirPCB(){

		char comandoRecibirPCB;
		recv(socketKernel,&comandoRecibirPCB,sizeof(char),0);
		log_info(loggerConPantalla, "Recibiendo PCB...\n");
		connectionHandlerKernel(socketKernel,comandoRecibirPCB);

}
void establecerPCB(){

	pcb_actual = recibirYDeserializarPcb(socketKernel);

	log_info(loggerConPantalla, "CPU recibe PCB correctamente\n");

	printf("\nPCB:%d\n", pcb_actual->pid);
	EjecutarProgramaMedianteAlgoritmo();


}





//----------------------------Manejo Memoria-------------------------------------
int conseguirDatosMemoria (char** instruccion, int paginaSolicitada,int offset,int size){
	int resultadoEjecucion;
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina

	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb_actual->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*instruccion=malloc((size+1)*sizeof(char));
	recv(socketMemoria,*instruccion,size,0);
	strcpy(*instruccion+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

int almacenarDatosEnMemoria(char* buffer, int size,int paginaAGuardar,int offset){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';
		send(socketMemoria,&comandoAlmacenar,sizeof(char),0);
		send(socketMemoria,&pcb_actual->pid,sizeof(int),0);
		send(socketMemoria,&paginaAGuardar,sizeof(int),0);
		send(socketMemoria,&offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		return resultadoEjecucion;

}
//----------------------------Manejo Memoria-------------------------------------

//----------------------------Manejo Instrucciones-------------------------------------
char* obtener_instruccion(){
	int program_counter = pcb_actual->programCounter;
	int byte_inicio_instruccion = pcb_actual->indiceCodigo[program_counter][0];
	int bytes_tamanio_instruccion = pcb_actual->indiceCodigo[program_counter][1];
	int num_pagina = byte_inicio_instruccion / config_paginaSize;
	int offset = byte_inicio_instruccion - (num_pagina * config_paginaSize);//no es 0 porque evita el begin
	char* mensajeRecibido;
	char* mensajeRecibido2;
	char* instruccion;
	char* continuacion_instruccion;
	int bytes_a_leer_primera_pagina;

	if (bytes_tamanio_instruccion > (config_paginaSize * 2)){
		printf("El tamanio de la instruccion es mayor al tamanio de pagina\n");
	}
	if ((offset + bytes_tamanio_instruccion) < config_paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_tamanio_instruccion)<0)
			{
			printf("No se pudo solicitar el contenido\n");
			}
			else{
				instruccion=mensajeRecibido;
				}
	} else {
		bytes_a_leer_primera_pagina = config_paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					{printf("No se pudo solicitar el contenido\n");}
		else{
				instruccion=mensajeRecibido;
			}
		//free(mensajeRecibido);
		log_info(loggerConPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido2,(num_pagina + 1),0,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
					{printf("No se pudo solicitar el contenido\n");}
						else{
						continuacion_instruccion=mensajeRecibido2;
						log_info(loggerConPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
								}

			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(loggerConPantalla, "La continuacion de la instruccion es 0. Ni la leo");
		}
	}
	char** string_cortado = string_split(instruccion, "\n");
	free(instruccion);
	instruccion= string_new();
	string_append(&instruccion, string_cortado[0]);

	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);

	return instruccion;
}
void EjecutarProgramaMedianteAlgoritmo(){

	int cantidadInstruccionesAEjecutarPcb_Actual=pcb_actual->cantidadInstrucciones;

	if(cantidadInstruccionesAEjecutarPorKernel==0){ //es FIFO
		while(cantidadInstruccionesAEjecutarPorKernel < cantidadInstruccionesAEjecutarPcb_Actual){

			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel++;
			cantidadIntruccionesEjecutadas++;
		}
	} else{
		while (cantidadInstruccionesAEjecutarPorKernel > 0){
			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel--;
			cantidadIntruccionesEjecutadas++;
		}
		expropiar();
	}
}
void ejecutarInstruccion(){


	char *orden = malloc(sizeof(char));
	*orden = '\0';
	char *instruccion = obtener_instruccion();

	log_warning(loggerConPantalla,"Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);

	recv(socketKernel,orden,sizeof(char),MSG_DONTWAIT); //espero sin bloquearme ordenes del kernel

	if(*orden == 'F') cpuExpropiada = -1;

	free(instruccion);

	pcb_actual->programCounter = pcb_actual->programCounter + 1;

	if(cpuExpropiada == -1 || cpuBloqueada == 0 || cpuFinalizada == 0){
		expropiar();
	}
}
//----------------------------Manejo Instrucciones-------------------------------------

//-----------------------------LOGS, CONFIGS Y SIGNALS------------------------------------------------------
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
void signalHandler(int signum)
{
    if (signum == SIGUSR1 || signum == SIGINT )
    {
    	log_warning(loggerConPantalla,"Cierre por signal, ejecutando ultimas instrucciones del proceso de PID %d y cerrando CPU ...",pcb_actual->pid);
    	cpuFinalizada=0;
    }
    if (signum == SIGUSR2){
    	log_warning(loggerConPantalla,"Se esta expropiando el proceso de PID %d ejecutando ultima instruccion y desalojandolo de CPU ...",pcb_actual->pid);
    	cpuExpropiada=0;
    }
}


//------------------------------MANEJO DE ESTADOS DE CPU--------------------------------------
void CerrarPorSignal(){
	char comandoInterruptHandler='X';
	char comandoCierreCpu='C';

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoCierreCpu,sizeof(char),0);
	 //hacer un send a memoria para avisar que se desconecto la CPU y que no se ponga como loca
	log_warning(loggerConPantalla,"Se ha desconectado CPU con signal correctamente");
	free(pcb_actual);
	exit(1);
}
void expropiar(){

	if(cpuExpropiada == -1) finalizar();
	if(cpuFinalizada == 0) CerrarPorSignal();
	if(cpuBloqueada == 0)log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado en la instruccion %d por semaforo negativo", pcb_actual->pid, pcb_actual->programCounter);
	else expropiarPorRR();


}

void expropiarPorRR(){

	char comandoExpropiarCpu = 'R';

	send(socketKernel,&comandoExpropiarCpu , sizeof(char),0);
	send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado en la instruccion %d por Fin de quantum", pcb_actual->pid, pcb_actual->programCounter);

	esperarPCB();
}

//------------------------------MANEJO DE ESTADOS DE CPU--------------------------------------



//-----------------------------PEDIDOS AL KERNEL-----------------------------------------
void enviarAlKernelPedidoDeNuevoProceso(int socketKernel){
	char comandoGetNuevoProceso = 'N';
	send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);
	log_info(loggerConPantalla,"Se hizo el pedido a Kernel para comenzar a ejecutar Script ANSISOP\n");
}
void recibirYMostrarAlgortimoDePlanificacion(int socketKernel){
	recv(socketKernel,&quantum,sizeof(int),0);

		if(quantum!=0){
			log_info(loggerConPantalla,"\nAlgoritmo: RR de Q:%d\n", quantum);
		}
		log_info(loggerConPantalla,"\nAlgoritmo FIFO\n");
}
//-----------------------------PEDIDOS AL KERNEL-----------------------------------------

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
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(loggerConPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(loggerConPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
}
int cantidadPaginasTotales(){
	int paginasTotales= (stackSize + pcb_actual->cantidadPaginasCodigo);
	return paginasTotales;
}



void stackOverflow(){
		log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d sufrio stack overflow", pcb_actual->pid);
		finalizar();

}

char* devolverStringFlags(t_banderas flags){
	char *flagsAConcatenar = string_new();
	if(flags.creacion==true){
		string_append(&flagsAConcatenar, "c");
	}
	if(flags.lectura==true){
		string_append(&flagsAConcatenar, "r");
	}
	if(flags.escritura==true){
		string_append(&flagsAConcatenar, "w");
	}

	return flagsAConcatenar;
}

