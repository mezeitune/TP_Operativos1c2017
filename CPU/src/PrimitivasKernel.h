//SEMAFOROS ANSISOP
void wait(t_nombre_semaforo identificador_semaforo){
	char comandoWait = 'W';
	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	int bloquearScriptONo;
	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(loggerConPantalla, "Semaforo a bajar: %s", string_cortado[0]);

	send(socketKernel,&comandoWait,sizeof(char),0);
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
	char comandoSignal = 'L';
	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(loggerConPantalla, "Semaforo a subir: %s", string_cortado[0]);
	send(socketKernel,&comandoSignal,sizeof(char),0);
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
	char comandoInterruptHandler = 'X';
	char comandoReservarMemoria = 'R';
	int pid = pcb_actual->pid;
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoReservarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&espacio,sizeof(int),0);
	recv(socketKernel,&pagina,sizeof(int),0);
	recv(socketKernel,&offset,sizeof(int),0);
	t_puntero puntero = pagina * config_paginaSize + offset;
	return puntero;
}
void liberar (t_puntero puntero){
	int num_pagina = puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);
	int pid = pcb_actual->pid;
	char comandoInterruptHandler = 'X';
	char comandoLiberarMemoria = 'L';
	int resultadoEjecucion;
	int tamanio = sizeof(t_puntero);

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoLiberarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&num_pagina,sizeof(int),0);
	send(socketKernel,&offset,tamanio,0);

	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(loggerConPantalla,"Se ha liberado correctamente el heap previamente reservado apuntando a %d",puntero);
	else
		log_info(loggerConPantalla,"No se ha podido liberar el heap apuntada por",puntero);
}
//HEAP


