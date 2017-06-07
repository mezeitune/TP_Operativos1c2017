void validarArchivoFunction(int socket_cliente){
	int tamanoArchivo;
	int validado;



	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
    void* nombreArchivo = malloc(tamanoArchivo);
    recv(socket_cliente,nombreArchivo,tamanoArchivo,0);

    printf("Recibi el nombre del archivo\n ");


	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../metadata/");
	string_append(&nombreArchivoRecibido, nombreArchivo);
    printf("%s", nombreArchivoRecibido);
	if( access(nombreArchivoRecibido , F_OK ) != -1 ) {
	    // file exists
		printf("\n el archivo existe\n");

		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
	} else {
	    // file doesn't exist
	   printf("\n Archivo inexistente");

	   validado=0;
	   send(socket_cliente,&validado,sizeof(int),0);
	}

    printf("\n ");//esto tiene que estar , no se por que
}


void crearArchivoFunction(int socket_cliente){
	FILE *fp;
	if( access( "../metadata/nuevo.bin", F_OK ) != -1 ) {
		//falta ver en este if de arriba tambien si el archivo existe y si esta en modo "c"
	}else{
		fp = fopen("../metadata/nuevo.bin", "ab+");//creo el archivo
		//falta que por default se le asigne un bloque a ese archivo
	}
}

void borrarArchivoFunction(int socket_cliente){
	FILE *fp;

	if( access( "../metadata/nuevo.bin", F_OK ) != -1 ) {


	   fp = fopen("../metadata/nuevo.bin", "w");


	   if(remove("../metadata/nuevo.bin") == 0)
	   {
	      printf("File deleted successfully");
	   }
	   else
	   {
	      printf("Error: unable to delete the file");
	   }
	} else {
	    // file doesn't exist
		printf("Archivo inexistente");
	}

	   //falta marcar los bloques como libres dentro del bitmap


}


void obtenerDatosArchivoFunction(int socket_cliente){
	FILE *fp;

	if( access( "../metadata/alumno.bin", F_OK ) != -1 ) {


		printFilePermissions("../metadata/alumno.bin");
		if((archivoEnModoLectura("../metadata/alumno.bin"))==1){
			//printf("\n dale sigamo");
			fp = fopen("../metadata/alumno.bin", "r");
			printf("\n %s",obtenerBytesDeUnArchivo(fp, 5, 9));


		}else{
			printf("\n El archivo no esta en modo lectura");
		}
	} else {
	    // file doesn't exist
		printf("Archivo inexistente");
	}


}




void guardarDatosArchivoFunction(int socket_cliente){
	FILE *fp;

	if( access( "../metadata/alumno.bin", F_OK ) != -1 ) {

					printFilePermissions("../metadata/alumno.bin");
					if(archivoEnModoEscritura("../metadata/alumno.bin")==1){
						printf("\n dale sigamo");
					}else{
						printf("\n El archivo no esta en modo escritura");
					}

				} else {
				    // file doesn't exist
					printf("Archivo inexistente");
				}


}

