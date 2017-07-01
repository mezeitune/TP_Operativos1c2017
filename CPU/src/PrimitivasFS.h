
//FS
t_descriptor_archivo abrir_archivo(t_direccion_archivo direccion, t_banderas flags){

	t_descriptor_archivo descriptorArchivoAbierto;
	int descriptor;
	char comandoCapaFS = 'F';
	char comandoAbrirArchivo = 'A';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoAbrirArchivo,sizeof(char),0);

	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	printf("Direccion: %s\n",direccion);
	int tamanoDireccion=sizeof(char)*strlen(direccion);
	send(socketKernel,&tamanoDireccion,sizeof(int),0);
	send(socketKernel,direccion,tamanoDireccion,0);

	printf("%s\n", flags.creacion ? "true" : "false");
	printf("%s\n", flags.escritura ? "true" : "false");
	printf("%s\n", flags.lectura ? "true" : "false");
	//enviar los flags al kernel
	char* flagsAEnviar;
	flagsAEnviar = devolverStringFlags(flags);

	/*TODO: HARCODEO LOS FLAGS*/
	char* flagHarcodeado = "rwc";
	printf("%s\n",flagsAEnviar);
	int tamanoFlags=sizeof(char)*strlen(flagHarcodeado);
	send(socketKernel,&tamanoFlags,sizeof(int),0);
	send(socketKernel,flagHarcodeado,tamanoFlags,0);

	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	printf("resultado:%d\n",resultadoEjecucion);
	//log_info(loggerConPantalla,"El proceso de PID %d ha abierto un archivo de descriptor %d en modo %s",pid,descriptor);

	if(resultadoEjecucion < 0){
	//	log_error(loggerConPantalla,"Error del proceso de PID %d al abrir un archivo de descriptor %d en modo %s",pid,descriptor);
		expropiarPorKernel();
		return 0;
	}
	recv(socketKernel,&descriptor,sizeof(int),0);
	descriptorArchivoAbierto = (t_descriptor_archivo) descriptor;
	printf("Descriptor :%d\n",descriptorArchivoAbierto);
	return descriptorArchivoAbierto;
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
	else {
		log_error(loggerConPantalla,"Error del proceso de PID %d al borrar el archivo de descriptor %d");
		expropiarPorKernel();
	}
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
	else {
		log_error(loggerConPantalla,"Error del proceso de PID %d ha cerrado el archivo de descriptor %d");
		expropiarPorKernel();
	}

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
	else {
		log_error(loggerConPantalla,"Error del proceso de PID %d al mover el cursor de un archivo de descriptor %d en la posicion %d");
		expropiarPorKernel();
	}
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
		recv(socketKernel,&tamanioInfoLeida,sizeof(int),0); /*TODO: No hace falta, ya es el tamano que recibis por parametro*/
		void* infoLeida = malloc(tamanioInfoLeida);
		recv(socketKernel,infoLeida,tamanioInfoLeida,0);
		char *infoLeidaChar = string_new();
		string_append(&infoLeidaChar, infoLeida);
		log_info(loggerConPantalla,"La informacion leida es %s",infoLeidaChar);
	}else{
		log_error(loggerConPantalla,"Error del proceso de PID %d al leer informacion de un archivo de descriptor %d en la posicion %d");
		expropiarPorKernel();
	}
}








void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	int descriptorHardcodeado = 3;
	/*if(descriptor_archivo==DESCRIPTOR_SALIDA){

		char comandoImprimir = 'X';
		char comandoImprimirPorConsola = 'P';
		send(socketKernel,&comandoImprimir,sizeof(char),0);
		send(socketKernel,&comandoImprimirPorConsola,sizeof(char),0);

		send(socketKernel,&tamanio,sizeof(int),0);
		send(socketKernel,(char*)informacion,tamanio,0);
		send(socketKernel,&pcb_actual->pid,sizeof(int),0);
	}else {
*/
		char comandoCapaFS = 'F';
			char comandoEscribirArchivo = 'G';
			int resultadoEjecucion ;
			send(socketKernel,&comandoCapaFS,sizeof(char),0);
			send(socketKernel,&comandoEscribirArchivo,sizeof(char),0);
			int pid= pcb_actual->pid;
			send(socketKernel,&pid,sizeof(int),0);
			send(socketKernel,&descriptorHardcodeado,sizeof(int),0);
			printf("Descriptor:%d\n",descriptorHardcodeado);
			send(socketKernel,&tamanio,sizeof(int),0);
			printf("Tamano:%d\n",tamanio);

			send(socketKernel,(char*)informacion,tamanio,0); //puntero que apunta a la direccion donde quiero obtener la informacion
			printf("Data:%s\n",(char*)informacion);
			recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
			if(resultadoEjecucion > 0)
			log_info(loggerConPantalla,"La informacion ha sido escrita con exito en el archivo de descriptor %d PID %d");
			else {
				log_error(loggerConPantalla,"Error del proceso de PID %d al escribir un archivo de descriptor %d ");
				expropiarPorKernel();
			}
	}
//}
//FS

