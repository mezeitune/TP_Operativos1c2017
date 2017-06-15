void printBitmap(){

	int j;
	for(j=0;j<32;j++){
		/*if(bitarray_test_bit(bitarray, j)==1){
			printf("ocupado-");
		}else{
			printf("liberado-");
		}*/
        bool a = bitarray_test_bit(bitarray,j);
        printf("%i", a);
	}
	//bitarray_clean_bit(bitarray,3);
	printf("\n");
}

void validarArchivoFunction(int socket_cliente){
	int tamanoArchivo;
	int validado;



	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
    void* nombreArchivo = malloc(tamanoArchivo);
    recv(socket_cliente,nombreArchivo,tamanoArchivo,0);

    printf("Recibi el nombre del archivo\n ");


	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../Archivos/");
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

	int tamanoArchivo;
	int validado;

	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoArchivo);
	recv(socket_cliente,nombreArchivo,tamanoArchivo,0);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	//Recorro bitmap y veo si hay algun bloque para asignarle
	//por default se le asigna un bloque al archivo recien creado
	int j;
	int encontroUnBloque=0;
	int bloqueEncontrado=0;
	for(j=0;j<cantidadBloques;j++){

        bool bit = bitarray_test_bit(bitarray,j);
        if(bit==0){
        	encontroUnBloque=1;
        	bloqueEncontrado=j;
        }
	}

	if(encontroUnBloque==0){
		fp = fopen(nombreArchivoRecibido, "ab+");
		//asignar bloque en el metadata del archivo

		validado=1;
		//send avisando al kernel que salio todo ok
	}else{
		validado=0;
		//send diciendo que hubo error
	}





}


void borrarArchivoFunction(int socket_cliente){
	FILE *fp;

	if( access( "../Archivos/nuevo.bin", F_OK ) != -1 ) {


	   fp = fopen("../Archivos/nuevo.bin", "w");


	   if(remove("../Archivos/nuevo.bin") == 0)
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

	if( access( "../Archivos/alumno.bin", F_OK ) != -1 ) {


		printFilePermissions("../Archivos/alumno.bin");
		if((archivoEnModoLectura("../Archivos/alumno.bin"))==1){
			//printf("\n dale sigamo");
			fp = fopen("../Archivos/alumno.bin", "r");
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

	if( access( "../Archivos/alumno.bin", F_OK ) != -1 ) {

					printFilePermissions("../Archivos/alumno.bin");
					if(archivoEnModoEscritura("../Archivos/alumno.bin")==1){
						printf("\n dale sigamo");
					}else{
						printf("\n El archivo no esta en modo escritura");
					}

				} else {
				    // file doesn't exist
					printf("Archivo inexistente");
				}


}

