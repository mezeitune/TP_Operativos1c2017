//SEMAFOROS ANSISOP
void wait(t_nombre_semaforo identificador_semaforo){
	char interruptHandler = 'X';
	char comandoWait = 'W';
	int pid = pcb_actual->pid;
	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	int bloquearScriptONo;
	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(loggerConPantalla, "Semaforo a bajar: %s", string_cortado[0]);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoWait,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,identificadorSemAEnviar,tamanio,0);

	recv(socketKernel,&bloquearScriptONo,sizeof(int),0);


	if(bloquearScriptONo < 0){
		cpuBloqueada = 0;
		serializarPcbYEnviar(pcb_actual, socketKernel);
		log_info(loggerConPantalla, "Script ANSISOP pid: %d bloqueado por semaforo: %s", pcb_actual->pid, string_cortado[0]);
		esperarPCB();
	}else {
		log_info(loggerConPantalla, "Script ANSISOP pid: %d sigue su ejecucion normal", pcb_actual->pid);
	}

	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);

}
void signal_Ansisop(t_nombre_semaforo identificador_semaforo){
	char interruptHandler = 'X';
	char comandoSignal = 'S';
	int pid = pcb_actual->pid;

	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(loggerConPantalla, "Semaforo a subir: %s", string_cortado[0]);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoSignal,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,identificadorSemAEnviar,tamanio,0);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
}
//SEMAFOROS ANSISOP

//HEAP
t_puntero reservar (t_valor_variable espacio){
	int pagina,offset;
	int resultadoEjecucion;
	char comandoInterruptHandler = 'X';
	char comandoReservarMemoria = 'R';
	int pid = pcb_actual->pid;
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoReservarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&espacio,sizeof(int),0);

	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion < 0) {
		expropiarPorKernel();
		return 0;
	}
	recv(socketKernel,&pagina,sizeof(int),0);
	recv(socketKernel,&offset,sizeof(int),0);

	t_puntero puntero = pagina * config_paginaSize + offset;
	printf("El puntero es %d",puntero);
	return puntero;
}
void liberar (t_puntero puntero){

	int num_paginaDelStack = puntero / config_paginaSize;
	int offsetDelStack = puntero - (num_paginaDelStack * config_paginaSize);
	int punteroHeap;
	char* mensajeRecibido;
	if ( conseguirDatosMemoria(&mensajeRecibido, num_paginaDelStack,offsetDelStack, sizeof(int))<0)
		{
		log_info(loggerConPantalla,"No se pudo solicitar el contenido\n");
		expropiarPorDireccionInvalida();
		}
		else{
			punteroHeap=atoi(mensajeRecibido);
		}
	int pid = pcb_actual->pid;
	char comandoInterruptHandler = 'X';
	char comandoLiberarMemoria = 'L';
	int resultadoEjecucion;
	int tamanio = sizeof(t_puntero);

	int num_paginaHeap = punteroHeap/ config_paginaSize;
	int offsetHeap = punteroHeap - (num_paginaHeap * config_paginaSize);

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoLiberarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&num_paginaHeap,sizeof(int),0);
	send(socketKernel,&offsetHeap,tamanio,0);

	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(loggerConPantalla,"Se ha liberado correctamente el heap previamente reservado apuntando a %d",punteroHeap);
	else
		log_info(loggerConPantalla,"No se ha podido liberar el heap apuntada por",punteroHeap);
}
//HEAP






t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	char interruptHandler = 'X';
	char comandoObtenerCompartida = 'O';
	int pid = pcb_actual->pid;
	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	int tamanio = sizeof(int)*strlen(variable_string);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoObtenerCompartida,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);


	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,variable_string,tamanio,0);
	free(variable_string);

	int valor_variable_int;
	recv(socketKernel,&valor_variable_int,sizeof(int),0);

	log_info(loggerConPantalla, "Valor de la variable compartida: %d", valor_variable_int);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor_variable_int;
}
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	char interruptHandler = 'X';
	char comandoAsignarCompartida = 'G';
	int pid = pcb_actual->pid;



	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	int tamanio = sizeof(int)*strlen(variable_string);

	log_info(loggerConPantalla, "Asignando el valor %d: de id: %s", valor,variable);
	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoAsignarCompartida,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,variable_string,tamanio,0);
	send(socketKernel,&valor,sizeof(int),0);
	free(variable_string);

	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor;
}
//SEMAFOROS ANSISOP

