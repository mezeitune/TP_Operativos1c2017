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

	if(encontroUnBloque==1){
		fp = fopen(nombreArchivoRecibido, "ab+");
		//asignar bloque en el metadata del archivo(y marcarlo como ocupado en el bitmap)
		//escribir el metadata ese del archivo (TAMANO y BLOQUES)
		bitarray_set_bit(bitarray,bloqueEncontrado);
		char *dataAPonerEnFile = string_new();
		string_append(&dataAPonerEnFile, "TAMANIO=");
		string_append(&dataAPonerEnFile, tamanioBloquesEnChar);
		string_append(&dataAPonerEnFile,"\n");
		string_append(&dataAPonerEnFile, "BLOQUES=");
		char* numerito=string_itoa(bloqueEncontrado);
		string_append(&dataAPonerEnFile,numerito);

		adx_store_data(nombreArchivoRecibido,dataAPonerEnFile);

		validado=1;
		//send avisando al kernel que salio todo ok
	}else{
		validado=0;
		//send diciendo que hubo error
	}





}


void borrarArchivoFunction(int socket_cliente){
	FILE *fp;

	int tamanoArchivo;
	int validado;

	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoArchivo);
	recv(socket_cliente,nombreArchivo,tamanoArchivo,0);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	if( access(nombreArchivoRecibido, F_OK ) != -1 ) {


	   fp = fopen(nombreArchivoRecibido, "w");
	   //poner en un array los bloques de ese archivo para luego liberarlos
	   char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);

	   if(remove(nombreArchivoRecibido) == 0)
	   {


		   validado=1;

		   //marcar los bloques como libres dentro del bitmap (recorriendo con un for el array que cree arriba)
		   int d=0;
		   while(!(arrayBloques[d] == NULL)){
			   int indice=atoi(arrayBloques[d]);
			   bitarray_clean_bit(bitarray,indice);
		      d++;
		   }

		   //send diciendo que se elimino correctamente el archivo
	   }
	   else
	   {
		   validado=0;
	      //send que no se pudo eliminar el archivo
	   }
	} else {
		validado=0;
		//send diciendo que hubo un error y no se pudo eliminar el archivo
	}




}


void obtenerDatosArchivoFunction(int socket_cliente){//ver tema puntero , si lo tenog que recibir o que onda
	FILE *fp;

	int tamanoArchivo;
	int validado;
	int offset;
	int size;

	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoArchivo);
	recv(socket_cliente,nombreArchivo,tamanoArchivo,0);
	recv(socket_cliente,&offset,sizeof(int),0);
	recv(socket_cliente,&size,sizeof(int),0);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	if( access(nombreArchivoRecibido, F_OK ) != -1 ) {


		fp = fopen("../Archivos/alumno.bin", "r");
		printf("\n %s",obtenerBytesDeUnArchivo(fp, 5, 9));


		//si todod ok
		validado=1;
		//send diciendo que todo esta ok
	} else {
		validado=0;
		//send diciendo que el archivo no existe
	}


}




void guardarDatosArchivoFunction(int socket_cliente){//ver tema puntero, si lo tengo que recibir o que onda
	FILE *fp;

	int tamanoArchivo;
	int validado;
	int offset;
	int size;
	int tamanoBuffer;


	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoArchivo);
	recv(socket_cliente,nombreArchivo,tamanoArchivo,0);
	recv(socket_cliente,&offset,sizeof(int),0);
	recv(socket_cliente,&size,sizeof(int),0);
	recv(socket_cliente,&tamanoBuffer,sizeof(int),0);
	void* buffer = malloc(tamanoBuffer);
	recv(socket_cliente,buffer,tamanoBuffer,0);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, "../Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	if( access( nombreArchivoRecibido, F_OK ) != -1 ) {

		//ver de asignar mas bloques en caso de ser necesario
		//si no hay mas bloques de los que se requieren hay que hacer un send tirando error
		//sino guardamos en los bloques deseados
		//actualizamos el bitmap
		//actualizamos el metadata del archivo con los nuevos bloques y el nuevo tamano del archivo
		//y enviamos un buen send

	} else {
		validado=0;
		//send diciendo que el archivo no existe
	}


}

