//----------------------------------Manejo PCB------------------------------------------

int cpuFinalizada = 0;

void esperarPCB(){

	cpuBloqueadaPorSemANSISOP = 1;
	cpuFinalizadaPorSignal = 1;

	while(!cpuFinalizada){

		log_info(logConsolaPantalla,"CPU Esperando un script\n");
		cantidadInstruccionesAEjecutarPorKernel = quantum;
		recibirPCB();
	}

	log_warning(logConsolaPantalla,"CPU finalizada\n");
}
void recibirPCB(){
		char comandoRecibirPCB;

		recv(socketKernel,&comandoRecibirPCB,sizeof(char),MSG_WAITALL);
		log_info(logConsolaPantalla, "Recibiendo PCB...\n");

		establecerPCB(socketKernel);
		EjecutarProgramaMedianteAlgoritmo();
		//connectionHandlerKernel(socketKernel,comandoRecibirPCB);
}
void establecerPCB(){

	pcb_actual = recibirYDeserializarPcb(socketKernel);
	/*TODO: Aca recibis quantum y quantum sleep*/
	recv(socketKernel,&retardo_entre_instruccion,sizeof(int),0);
	recibiPcb=0;

	log_info(logConsolaPantalla, "CPU recibe PCB de PID %d correctamente\n",pcb_actual->pid);
}

void connectionHandlerKernel(int socketAceptado, char orden) {

	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	switch (orden) {
		case 'S':
			log_info(logConsolaPantalla, "Se esta por asignar un PCB");

			establecerPCB(socketAceptado);
					break;
		default:
				if(orden == '\0') break;
				log_warning(logConsolaPantalla,"\nOrden %c no definida\n", orden);
				break;
	}
	orden = '\0';
	return;
}
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(logConsolaPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(logConsolaPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
}
