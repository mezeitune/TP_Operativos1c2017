//----------------------------------Manejo PCB------------------------------------------
void esperarPCB(){

	while(cpuOcupada==1 ){
		printf("%d",cpuFinalizada);
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
void establecerPCB(){

	pcb_actual = recibirYDeserializarPcb(socketKernel);
	recibiPcb=0;
	log_info(loggerConPantalla, "CPU recibe PCB correctamente\n");

	printf("\nPCB:%d\n", pcb_actual->pid);
	EjecutarProgramaMedianteAlgoritmo();


}
