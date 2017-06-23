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



//FS
t_descriptor_archivo abrir_archivo(t_direccion_archivo direccion, t_banderas flags){

	t_descriptor_archivo descriptorArchivoAbierto;
	char comandoCapaFS = 'F';
	char comandoAbrirArchivo = 'A';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoAbrirArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	int tamanoDireccion=sizeof(int)*strlen(direccion);
	send(socketKernel,&tamanoDireccion,sizeof(int),0);
	send(socketKernel,direccion,tamanoDireccion,0);

	//enviar los flags al kernel
	char* flagsAEnviar;
	flagsAEnviar = devolverStringFlags(flags);
	int tamanoFlags=sizeof(int)*strlen(flagsAEnviar);
	send(socketKernel,&tamanoFlags,sizeof(int),0);
	send(socketKernel,flagsAEnviar,tamanoFlags,0);


	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);


	if(resultadoEjecucion==1){
		recv(socketKernel,&descriptorArchivoAbierto,sizeof(int),0);
		log_info(loggerConPantalla,"El proceso de PID %d ha abierto un archivo de descriptor %d en modo %s");
		return descriptorArchivoAbierto;
	}
	else {

		log_info(loggerConPantalla,"Error del proceso de PID %d al abrir un archivo de descriptor %d en modo %s");
		return 0;
	}
}

void borrar_archivo (t_descriptor_archivo descriptor_archivo){

	char comandoCapaFS = 'F';
	char comandoBorrarArchivo = 'B';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoBorrarArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(loggerConPantalla,"El proceso de PID %d ha borrado un archivo de descriptor %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d al borrar el archivo de descriptor %d");
}

void cerrar_archivo(t_descriptor_archivo descriptor_archivo){

	char comandoCapaFS = 'F';
	char comandoCerrarArchivo = 'P';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoCerrarArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
	log_info(loggerConPantalla,"El proceso de PID %d ha cerrado un archivo de descriptor %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d ha cerrado el archivo de descriptor %d");

}


void moverCursor_archivo (t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

	char comandoCapaFS = 'F';
	char comandoMoverCursorArchivo = 'M';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoMoverCursorArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&posicion,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
	log_info(loggerConPantalla,"El proceso de PID %d ha movido el cursor de un archivo de descriptor %d en la posicion %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d al mover el cursor de un archivo de descriptor %d en la posicion %d");
}
void leer_archivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){

	char comandoCapaFS = 'F';
	char comandoLeerArchivo = 'O';
	int resultadoEjecucion ;
	int tamanioInfoLeida;

	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoLeerArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&informacion,sizeof(int),0); //puntero que apunta a la direccion donde quiero obtener la informacion
	send(socketKernel,&tamanio,sizeof(int),0); //tamanio de la instruccion en bytes que quiero leer
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);

	if(resultadoEjecucion==1){
		recv(socketKernel,&tamanioInfoLeida,sizeof(int),0);
		void* infoLeida = malloc(tamanioInfoLeida);
		recv(socketKernel,&infoLeida,tamanioInfoLeida,0);
		char *infoLeidaChar = string_new();
		string_append(&infoLeidaChar, infoLeida);
		log_info(loggerConPantalla,"La informacion leida es %s",infoLeidaChar);
	}else{
		log_info(loggerConPantalla,"Error del proceso de PID %d al leer informacion de un archivo de descriptor %d en la posicion %d");
	}
}


void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	if(descriptor_archivo==DESCRIPTOR_SALIDA){

		char comandoImprimir = 'X';
		char comandoImprimirPorConsola = 'P';
		send(socketKernel,&comandoImprimir,sizeof(char),0);
		send(socketKernel,&comandoImprimirPorConsola,sizeof(char),0);

		send(socketKernel,&tamanio,sizeof(int),0);
		send(socketKernel,(char*)informacion,tamanio,0);
		send(socketKernel,&pcb_actual->pid,sizeof(int),0);
	}else {

			char comandoCapaFS = 'F';
			char comandoEscribirArchivo = 'G';
			int resultadoEjecucion ;
			send(socketKernel,&comandoCapaFS,sizeof(char),0);
			send(socketKernel,&comandoEscribirArchivo,sizeof(char),0);
			int pid= pcb_actual->pid;
			send(socketKernel,&pid,sizeof(int),0);
			send(socketKernel,&descriptor_archivo,sizeof(int),0);
		//	send(socketKernel,&valor,sizeof(int),0); //puntero que apunta a la direccion donde quiero obtener la informacion
			send(socketKernel,&tamanio,sizeof(int),0);
			recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
			if(resultadoEjecucion==1)
			log_info(loggerConPantalla,"La informacion ha sido escrita con exito en el archivo de descriptor %d PID %d");
			else log_info(loggerConPantalla,"Error del proceso de PID %d al escribir un archivo de descriptor %d ");
	}
}
//FS

