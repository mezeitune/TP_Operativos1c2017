#include "PrimitivasDeInstrucciones.h"
#include "PrimitivasKernel.h"
#include "ManejoInstrucciones.h"
#include "Lecto-EscrituraMemoria.h"
#include "ManejoPCB.h"
#include "PrimitivasFS.h"
#include "LogsConfigsSignals.h"


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
	while(cpuFinalizada!=0){
	esperarPCB();
	}
	CerrarPorSignal();
	return 0;
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

	if(cpuExpropiada == -1) expropiarPorKernel();
	if(cpuFinalizada == 0) CerrarPorSignal();
	if(cpuBloqueada == 0)log_warning(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado en la instruccion %d por semaforo negativo", pcb_actual->pid, pcb_actual->programCounter);
	else expropiarPorRR();


}
void expropiarPorKernel(){
	log_error(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por Kernel\n", pcb_actual->pid);
	serializarPcbYEnviar(pcb_actual,socketKernel);

	free(pcb_actual);

	cpuExpropiada = 1;
	cpuOcupada=1;
	esperarPCB();
}

void expropiarPorStackOverflow(){
	char interruptHandler= 'X';
	char caseStackOverflow = 'S';
	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&caseStackOverflow,sizeof(char),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	log_info(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por StackOverflow\n", pcb_actual->pid);

	free(pcb_actual);

	cpuExpropiada = 1;
	cpuOcupada=1;
	esperarPCB();
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

