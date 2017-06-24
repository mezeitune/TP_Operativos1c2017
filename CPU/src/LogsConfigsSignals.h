//-----------------------------LOGS, CONFIGS Y SIGNALS------------------------------------------------------
void leerConfiguracion(char* ruta) {
	configuracion_memoria = config_create(ruta);
	ipMemoria = config_get_string_value(configuracion_memoria, "IP_MEMORIA");
	puertoKernel= config_get_string_value(configuracion_memoria, "PUERTO_KERNEL");
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO_MEMORIA");
	ipKernel = config_get_string_value(configuracion_memoria,"IP_KERNEL");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nPUERTO KERNEL:%s\nIP KERNEL:%s\nPUERTO MEMORIA:%s\nIP MEMORIA:%s\n",puertoKernel,ipKernel,puertoMemoria,ipMemoria);
	printf("---------------------------------------------------\n");
}
void inicializarLog(char *rutaDeLog){

	mkdir("/home/utnso/Log",0755);

	loggerSinPantalla = log_create(rutaDeLog,"CPU", false, LOG_LEVEL_INFO);
	loggerConPantalla = log_create(rutaDeLog,"CPU", true, LOG_LEVEL_INFO);
}
void signalHandler(int signum)
{
    if (signum == SIGUSR1 || signum == SIGINT )
    {
    	log_warning(loggerConPantalla,"Cierre por signal, ejecutando ultimas instrucciones del proceso de PID %d y cerrando CPU ...",pcb_actual->pid);
    	cpuFinalizada=0;
    }
    if (signum == SIGUSR2){
    	log_warning(loggerConPantalla,"Se esta expropiando el proceso de PID %d ejecutando ultima instruccion y desalojandolo de CPU ...",pcb_actual->pid);
    	cpuExpropiada=0;
    }
}
