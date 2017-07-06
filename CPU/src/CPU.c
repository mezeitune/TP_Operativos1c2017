#include "PrimitivasDeInstrucciones.h"
#include "PrimitivasKernel.h"
#include "ManejoInstrucciones.h"
#include "Lecto-EscrituraMemoria.h"
#include "ManejoPCB.h"
#include "PrimitivasFS.h"
#include "LogsConfigsSignals.h"
//void* atenderInterrupciones();


int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	//socketInterrupciones = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);

	log_info(loggerConPantalla, "Inicia proceso CPU");

	signal(SIGUSR1, signalHandler);
	signal(SIGINT, signalHandler);
	recibirTamanioPagina(socketKernel);
	enviarAlKernelPedidoDeNuevoProceso(socketKernel);
	recibirYMostrarAlgortimoDePlanificacion(socketKernel);

	//pthread_create(&lineaInterrupciones,NULL,atenderInterrupciones,NULL);
	esperarPCB();

	return 0;
}



/*void* atenderInterrupciones(){
	char interrupcion;

	while(1){
	recv(socketInterrupciones,&interrupcion,sizeof(char),0);

	//if(interrupcion == 'F') ;

	}
}*/


//------------------------------EXPROPIAR PROCESOS-------------------------------------
void CerrarPorSignal(){

	char comandoInterruptHandler='X';
	char comandoCierreCpu='C';
	char comandoCerrarMemoria = 'X';

	if(quantum > 0 && recibiPcb == 0){
		expropiarPorRRYCerrar();
	}
		send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
		send(socketKernel,&comandoCierreCpu,sizeof(char),0);
		send(socketMemoria,&comandoCerrarMemoria,sizeof(char),0);

	if(recibiPcb == 0){
		serializarPcbYEnviar(pcb_actual, socketKernel);
	}

	//shutdown(socketKernel,1);
	///shutdown(socketInterrupciones,1);
	//close(socketKernel);
	//close(socketInterrupciones);
	//close(socketMemoria);
	log_warning(loggerConPantalla,"Se ha desconectado CPU con signal correctamente");
	free(pcb_actual);
	exit(1);
}
void expropiarVoluntariamente(){

	if(cpuExpropiadaPorKernel == -1) expropiarPorKernel();
	if(cpuBloqueadaPorSemANSISOP == 0) log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado en la instruccion %d por semaforo negativo", pcb_actual->pid, pcb_actual->programCounter);
	else expropiarPorRR();


}
void expropiarPorKernel(){
	log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por Kernel", pcb_actual->pid);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
	free(pcb_actual);
	recibiPcb=1;
	cpuExpropiadaPorKernel = 1;
	cpuOcupada=1;
	esperarPCB();
}
void expropiarPorDireccionInvalida(){
	log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por intentar acceder a una referencia en memoria invalida", pcb_actual->pid);
	char interruptHandler= 'X';
	char caseDireccionInvalida= 'M';

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&caseDireccionInvalida,sizeof(char),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	log_info(loggerSinPantalla, "La CPU ha enviado el  PCB serializado al kernel");
	send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
	log_info(loggerSinPantalla, "La CPU ha enviado las instrucciones ejecutadas al kernel");
	free(pcb_actual);
	recibiPcb=1;
	cpuExpropiadaPorKernel = 1;
	cpuOcupada=1;
	esperarPCB();
}
void expropiarPorStackOverflow(){
	char interruptHandler= 'X';
	char caseStackOverflow = 'K';

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&caseStackOverflow,sizeof(char),0);

	serializarPcbYEnviar(pcb_actual,socketKernel);
	log_info(loggerSinPantalla, "La CPU ha enviado el  PCB serializado al kernel");
	send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
	log_info(loggerSinPantalla, "La CPU ha enviado las instrucciones ejecutadas al kernel");
	log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por StackOverflow\n", pcb_actual->pid);

	free(pcb_actual);
	recibiPcb=1;
	cpuExpropiadaPorKernel = 1;
	cpuOcupada=1;
	esperarPCB();
}

void expropiarPorRR(){

	char comandoExpropiarCpu = 'R';
	if(cpuBloqueadaPorSemANSISOP != 0){

		send(socketKernel,&comandoExpropiarCpu , sizeof(char),0);
		send(socketKernel, &cpuFinalizadaPorSignal, sizeof(int),0);
		send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
		serializarPcbYEnviar(pcb_actual,socketKernel);
		log_info(loggerSinPantalla, "La CPU ha enviado el  PCB serializado al kernel");
		log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado en la instruccion %d por Fin de quantum", pcb_actual->pid, pcb_actual->programCounter);
		free(pcb_actual);
		esperarPCB();
	}
}
void expropiarPorRRYCerrar(){
	char comandoExpropiarCpu = 'R';

	if(pcb_actual->programCounter == pcb_actual->cantidadInstrucciones){//Por si justo el quantum toca en el end
		return;
	}
	send(socketKernel,&comandoExpropiarCpu , sizeof(char),0);
	send(socketKernel, &cpuFinalizadaPorSignal, sizeof(int),0);
	send(socketKernel,&cantidadIntruccionesEjecutadas,sizeof(int),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	log_info(loggerSinPantalla, "La CPU ha enviado el  PCB serializado al kernel");
	return;
}
//------------------------------EXPROPIAR PROCESOS--------------------------------------



//-----------------------------PEDIDOS AL KERNEL-----------------------------------------
void enviarAlKernelPedidoDeNuevoProceso(int socketKernel){
	char comandoGetNuevoProceso = 'N';
	send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);
	log_info(loggerConPantalla,"Se hizo el pedido a Kernel para comenzar a ejecutar Script ANSISOP\n");
}
void recibirYMostrarAlgortimoDePlanificacion(int socketKernel){
	recv(socketKernel,&quantum,sizeof(int),0);

		if(quantum==0){
			log_info(loggerConPantalla,"\nAlgoritmo FIFO\n");
			return;
		}else log_info(loggerConPantalla,"\nAlgoritmo: RR de Q:%d\n", quantum);
		return;
}
//-----------------------------PEDIDOS AL KERNEL-----------------------------------------


int cantidadPaginasTotales(){
	int paginasTotales= (config_stackSize + pcb_actual->cantidadPaginasCodigo);
	return paginasTotales;
}



void stackOverflow(){
		log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d sufrio stack overflow", pcb_actual->pid);
		expropiarPorStackOverflow();

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

