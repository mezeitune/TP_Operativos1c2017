//----------------------------Manejo Instrucciones-------------------------------------
char* obtener_instruccion(){
	log_info(loggerSinPantalla,"Obteniendo una instruccion del PC= %d", pcb_actual->programCounter);
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
		log_info(loggerConPantalla,"El tamanio de la instruccion es mayor al tamanio de pagina\n");
		expropiarPorDireccionInvalida();
	}
	if ((offset + bytes_tamanio_instruccion) < config_paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_tamanio_instruccion)<0)
			{
			log_info(loggerConPantalla,"No se pudo solicitar el contenido\n");
			expropiarPorDireccionInvalida();
			}
			else{
				instruccion=mensajeRecibido;
				}
	} else {
		bytes_a_leer_primera_pagina = config_paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					{
					log_info(loggerConPantalla,"No se pudo solicitar el contenido\n");
					expropiarPorDireccionInvalida();
					}
		else{
				instruccion=mensajeRecibido;
			}

		log_info(loggerConPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido2,(num_pagina + 1),0,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
					{log_info(loggerConPantalla,"No se pudo solicitar el contenido\n");
					expropiarPorDireccionInvalida();}
						else{
						continuacion_instruccion=mensajeRecibido2;
						log_info(loggerConPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
								}

			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(loggerSinPantalla, "La continuacion de la instruccion es 0. Ni la leo");
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

	cantidadInstruccionesAEjecutarDelPcbActual = pcb_actual->cantidadInstrucciones;
	log_info(loggerConPantalla,"La cantidad de instrucciones a ejecutar son %d",cantidadInstruccionesAEjecutarDelPcbActual);
	if(cantidadInstruccionesAEjecutarPorKernel==0){ //es FIFO
		while(cantidadInstruccionesAEjecutarPorKernel < cantidadInstruccionesAEjecutarDelPcbActual || cpuBloqueadaPorSemANSISOP != 0){

			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel++; //para FIFO en si
			cantidadIntruccionesEjecutadas++;//para contabilidad del kernel
			log_info(loggerSinPantalla,"cantidad de instrucciones ejecutadas %d", cantidadIntruccionesEjecutadas);

		}
	} else{//es RR con quantum = cantidadInstruccionesAEjecutarPorKernel
		while (cantidadInstruccionesAEjecutarPorKernel > 0 && cpuBloqueadaPorSemANSISOP != 0){
			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel--; //voy decrementando el Quantum que me dio el kernel hasta llegar a 0
			log_info(loggerSinPantalla,"Quedan por ejecutar %d instrucciones", cantidadInstruccionesAEjecutarPorKernel);
			cantidadIntruccionesEjecutadas++;////para contabilidad del kernel
			log_info(loggerSinPantalla,"cantidad de instrucciones ejecutadas %d", cantidadIntruccionesEjecutadas);
		}
		//Este if va por el RR
		if(cpuFinalizadaPorSignal == 0) CerrarPorSignal();
		else expropiarVoluntariamente();
	}
}
void ejecutarInstruccion(){


	char orden;
	orden = '\0';
	char *instruccion = obtener_instruccion();
	retardo_entre_instruccion=retardo_entre_instruccion/1000;
	sleep(retardo_entre_instruccion);
	log_info(loggerSinPantalla,"El sleep es : %d ms",retardo_entre_instruccion);
	log_warning(loggerConPantalla,"Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);

	recv(socketKernel,&orden,sizeof(char),MSG_DONTWAIT); //espero sin bloquearme ordenes del kernel

	if(orden == 'F') cpuExpropiadaPorKernel = -1;


	free(instruccion);

	pcb_actual->programCounter = pcb_actual->programCounter + 1;

	if(cpuExpropiadaPorKernel == -1 || cpuBloqueadaPorSemANSISOP == 0){
		expropiarVoluntariamente();
	}
}
//----------------------------Manejo Instrucciones-------------------------------------
