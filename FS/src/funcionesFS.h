char *puertoFS;
char *puntoMontaje;
char *puerto_Kernel;
int tamanioBloques;
int cantidadBloques;
char* magicNumber;
char *ipFS;

t_config* configuracion_FS;

void leerConfiguracion(char* ruta){
	configuracion_FS = config_create(ruta);

	puertoFS= config_get_string_value(configuracion_FS,"PUERTO_FS");
	ipFS= config_get_string_value(configuracion_FS, "IP_FS");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");
	puerto_Kernel= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
}





void leerConfiguracionMetadata(char* ruta){
	configuracion_FS = config_create(ruta);

	tamanioBloques= config_get_string_value(configuracion_FS,"TAMANIO_BLOQUES");
	cantidadBloques= config_get_string_value(configuracion_FS, "CANTIDAD_BLOQUES");
	magicNumber = config_get_string_value(configuracion_FS,"MAGIC_NUMBER");


}


void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP FS:%s\nPUERTO FS:%s\nPUNTO MONTAJE:%s\n",ipFS,puertoFS,puntoMontaje);
		printf("\n \nTAMANIO BLOQUS:%s\nCANTIDAD BLQOUES:%s\nMAGIC NUMBER:%s\n",tamanioBloques,cantidadBloques,magicNumber);
		printf("---------------------------------------------------\n");
}



