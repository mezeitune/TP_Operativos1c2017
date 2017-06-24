//----------------------------------Manejo PCB------------------------------------------
void esperarPCB(){

	while(cpuOcupada==1 || cpuFinalizada==1){
		log_info(loggerConPantalla," CPU Esperando un script");
		cantidadInstruccionesAEjecutarPorKernel = quantum;
		recibirPCB();
		cpuOcupada--;
	}
	CerrarPorSignal();
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
