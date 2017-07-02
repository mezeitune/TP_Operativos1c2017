#include "sockets.h"
#include <commons/log.h>
#include "logger.h"
void printBitmap(){

	int j;
	for(j=0;j<cantidadBloques;j++){
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
    char* nombreArchivo = malloc(tamanoArchivo*sizeof(char) + sizeof(char));
    recv(socket_cliente,nombreArchivo,tamanoArchivo,0);
    strcpy(nombreArchivo + tamanoArchivo,"\0");
    log_info(loggerConPantalla,"Validando existencia de archivo--->Nombre:%s",nombreArchivo);


	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);
    printf("%s\n", nombreArchivoRecibido);
	if( access(nombreArchivoRecibido , F_OK ) != -1 ) {
	    // file exists
		log_info(loggerConPantalla,"El archivo existe");
		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
	} else {
	    // file doesn't exist
	  log_warning(loggerConPantalla,"El archivo no existe");
	   validado=-1;
	   send(socket_cliente,&validado,sizeof(int),0);
	}

}


void crearArchivoFunction(int socket_cliente){
	FILE *fp;

	int tamanoArchivo;
	int validado;

	recv(socket_cliente,&tamanoArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoArchivo);
	recv(socket_cliente,nombreArchivo,tamanoArchivo,0);
	strcpy(nombreArchivo + tamanoArchivo, "\0");
	log_info(loggerConPantalla,"Creando archivo--->Nombre:%s",nombreArchivo);
	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
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
        	break;
        }
	}

	if(encontroUnBloque==1){
		fp = fopen(nombreArchivoRecibido, "ab+");
		//asignar bloque en el metadata del archivo(y marcarlo como ocupado en el bitmap)
		//escribir el metadata ese del archivo (TAMANO y BLOQUES)

		bitarray_set_bit(bitarray,bloqueEncontrado);

		char *dataAPonerEnFile = string_new();
		string_append(&dataAPonerEnFile, "TAMANIO=");
		string_append(&dataAPonerEnFile, "0");
		string_append(&dataAPonerEnFile,"\n");
		string_append(&dataAPonerEnFile, "BLOQUES=[");
		char* numerito=string_itoa(bloqueEncontrado);
		string_append(&dataAPonerEnFile,numerito);
		string_append(&dataAPonerEnFile,"]");

		adx_store_data(nombreArchivoRecibido,dataAPonerEnFile);

		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
		printf("Se creo el archivo\n");
	}else{
		validado=0;
		send(socket_cliente,&validado,sizeof(int),0);
		printf("No se creo el archivo\n");
	}

}


void borrarArchivoFunction(int socket_cliente){
	FILE *fp;

	int tamanoNombreArchivo;
	int validado;

	recv(socket_cliente,&tamanoNombreArchivo,sizeof(int),0);
	void* nombreArchivo = malloc(tamanoNombreArchivo);
	recv(socket_cliente,nombreArchivo,tamanoNombreArchivo,0);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
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
		   send(socket_cliente,&validado,sizeof(char),0);
		   //send diciendo que se elimino correctamente el archivo
	   }
	   else
	   {
		   validado=0;
		   send(socket_cliente,&validado,sizeof(char),0);
	      //send que no se pudo eliminar el archivo
	   }
	} else {
		validado=0;
		send(socket_cliente,&validado,sizeof(char),0);
		//send diciendo que hubo un error y no se pudo eliminar el archivo
	}




}


void obtenerDatosArchivoFunction(int socket_cliente){//ver tema puntero , si lo tenog que recibir o que onda
	FILE *fp;

	int tamanoNombreArchivo;
	int validado;
	int cursor;
	int size;

	recv(socket_cliente,&tamanoNombreArchivo,sizeof(int),0);
	char* nombreArchivo = malloc(tamanoNombreArchivo + sizeof(char));
	recv(socket_cliente,nombreArchivo,tamanoNombreArchivo,0);
	strcpy(nombreArchivo + tamanoNombreArchivo, "\0");
	recv(socket_cliente,&cursor,sizeof(int),0);
	recv(socket_cliente,&size,sizeof(int),0);

	log_info(loggerConPantalla,"Obteniendo datos--->Archivo:%s--->Cursor:%d--->Size:%d",nombreArchivo,cursor,size);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);


	if( access(nombreArchivoRecibido, F_OK ) != -1 ) {


		fp = fopen(nombreArchivoRecibido, "r");
		printf("Abri el archivo\n");
		char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);
		printf("Obtuve el array de bloques\n");
		   int d=0;
		   int u=1;
		   int offYSize=cursor+size;
		   printf("size:%d\n",size);
		   printf("Tamanio bloque:%d\n",tamanioBloques);

		   int cantidadBloquesQueNecesito; /*TODO: Emprolijar*/
		   if((size%tamanioBloques) == 0) cantidadBloquesQueNecesito = 1;
		   if((size%tamanioBloques) < tamanioBloques) cantidadBloquesQueNecesito = 1;
		   if((size%tamanioBloques) > tamanioBloques){
			   cantidadBloquesQueNecesito = size / tamanioBloques;
		   }
/*		   if((size%tamanioBloques)!=0){
				cantidadBloquesQueNecesito++;
			}
*/
		   printf("Cantidad de bloques que necesito leer :%d\n",cantidadBloquesQueNecesito);

		   char* infoTraidaDeLosArchivos = string_new();
		   int hizoLoQueNecesita=0;
		   while(!(arrayBloques[d] == NULL)){
			   printf("Leyendo data del bloque:%s\n",arrayBloques[d]);
			   if(cursor<=(tamanioBloques*u)){
				   printf("Entre al if\n");
				   int t;
				   int inicial=d;
				   for(t=inicial;t<((inicial+cantidadBloquesQueNecesito));t++){ /*TODO:t<((inicial+cantidadBloquesQueNecesito)+1) Estaba asi, pero rompia mati*/
					   printf("Entre al for\n");
					   hizoLoQueNecesita=1;
					   int indice=atoi(arrayBloques[t]);
						char *nombreBloque = string_new();
						string_append(&nombreBloque, puntoMontaje);
						string_append(&nombreBloque, "Bloques/");
						string_append(&nombreBloque, arrayBloques[t]);
						string_append(&nombreBloque, ".bin");

						FILE *bloque=fopen(nombreBloque, "r");
						printf("Abri el bloque\n");
						if(t==(d+cantidadBloquesQueNecesito)){
							printf("Entre al primer if\n");
							int sizeQuePido=size-cursor;
							int offsetQuePido=0;
							char* data=obtenerBytesDeUnArchivo(bloque,offsetQuePido,sizeQuePido); /*TODO: Te cambio para que leea el bloque, y no el archivo en si*/
							string_append(&infoTraidaDeLosArchivos,data);
						}else if(t==inicial){
							printf("Entre al segundo if\n");
							int offsetQuePido=cursor-(tamanioBloques*u);
							int sizeQuePido=tamanioBloques-offsetQuePido;
							string_append(&infoTraidaDeLosArchivos,obtenerBytesDeUnArchivo(bloque,offsetQuePido , sizeQuePido));

						}else{
							printf("Entre al tercer if\n");
							int sizeQuePido=tamanioBloques;
							int offsetQuePido=0;
							string_append(&infoTraidaDeLosArchivos,obtenerBytesDeUnArchivo(bloque,offsetQuePido , sizeQuePido));

						}


				   }

			   }
			   if(hizoLoQueNecesita==1){
				   break;
			   }
		      d++;
		      u++;
		   }
		//printf("\n %s",obtenerBytesDeUnArchivo(fp, 5, 9));

		   printf("La info leida es:%s\n",infoTraidaDeLosArchivos);
		//si todod ok
		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
		send(socket_cliente,infoTraidaDeLosArchivos,size,0);
	} else {
		validado=-1;
		send(socket_cliente,&validado,sizeof(int),0); //El archivo no existe
	}


}




void guardarDatosArchivoFunction(int socket_cliente){//ver tema puntero, si lo tengo que recibir o que onda
	FILE *fp;

	int tamanoNombreArchivo;
	int validado;
	int puntero;
	int tamanoBuffer;


	recv(socket_cliente,&tamanoNombreArchivo,sizeof(int),0);
	printf("Tamano nombre archivo:%d\n",tamanoNombreArchivo);
	char* nombreArchivo = malloc(tamanoNombreArchivo);

	recv(socket_cliente,nombreArchivo,tamanoNombreArchivo,0);
	strcpy(nombreArchivo + tamanoNombreArchivo, "\0");
	printf("Nombre archivo:%s\n",nombreArchivo);

	recv(socket_cliente,&puntero,sizeof(int),0);
	printf("Puntero:%d\n",puntero);

	recv(socket_cliente,&tamanoBuffer,sizeof(int),0);
	printf("Tamano de la data:%d\n",tamanoBuffer);
	char* buffer = malloc(tamanoBuffer + sizeof(char));

	recv(socket_cliente,buffer,tamanoBuffer,0);
	strcpy(buffer + tamanoBuffer,"\0");
	printf("Data :%s\n",buffer);

	log_info(loggerConPantalla,"Guardando datos--->Archivo:%s--->Informacion:%s",nombreArchivo,buffer);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	printf("Toda la ruta :%s\n",nombreArchivoRecibido);


	if( access( nombreArchivoRecibido, F_OK ) != -1 ) {


		char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);


		int d=0;
		int cantidadBloques = 0;
		while(!(arrayBloques[d] == NULL)){
			printf("%s \n",arrayBloques[d]);
			d++;
			cantidadBloques ++;
		}
		d--;

		printf("Cantidad de bloques :%d\n",cantidadBloques);


		char *nombreBloque = string_new();
		string_append(&nombreBloque, puntoMontaje);
		string_append(&nombreBloque, "Bloques/");
		string_append(&nombreBloque, arrayBloques[d]);
		string_append(&nombreBloque, ".bin");
		printf("Nombre del ultimo bloque: %s\n",nombreBloque);
		//ver de asignar mas bloques en caso de ser necesario
		printf("Tamano del archivo : %d\n",atoi(obtTamanioArchivo(nombreArchivoRecibido)));
		printf("Tamano del bloque: %d\n",tamanioBloques);
		int cantRestante=tamanioBloques-(atoi(obtTamanioArchivo(nombreArchivoRecibido))-((cantidadBloques-1)*tamanioBloques));
		printf("Cantidad restante :%d\n",cantRestante);
		if(tamanoBuffer<cantRestante){
			adx_store_data(nombreBloque,buffer);
			log_info(loggerConPantalla,"Datos guardados--->Archivo:%s--->Informacion:%s",nombreArchivo,buffer);
			//send diciendo que todo esta bien
		}else{
			int cuantosBloquesMasNecesito=tamanoBuffer/tamanioBloques;
			if((tamanoBuffer%tamanioBloques)>0){
				cuantosBloquesMasNecesito++;
			}
			//si no hay mas bloques de los que se requieren hay que hacer un send tirando error
			int j;
			int r=0;
			int bloquesEncontrados=0;
			int bloqs[cuantosBloquesMasNecesito];
			for(j=0;j<cantidadBloques;j++){
		        bool bit = bitarray_test_bit(bitarray,j);
		        if(bit==0){
		        	//guardar en array bloqs los bloques que voy encontrando
		        	if(r==cuantosBloquesMasNecesito){
		        		break;
		        	}else{
		            	bloqs[r]=j;
		            	r++;
		        	}
		        	bloquesEncontrados++;
		        }
		        printf("PASO POR ACAAAAAAAAAA3 \n");
			}

			if(bloquesEncontrados>=cuantosBloquesMasNecesito){
				//guardamos en los bloques deseados

				int s;
				char* loQueVaQuedandoDeBuffer=(char*)buffer;
				for(s=0;s<cuantosBloquesMasNecesito;s++){

					char *nombreBloque = string_new();
					string_append(&nombreBloque, puntoMontaje);
					string_append(&nombreBloque, "Bloques/");
					char* numerito=string_itoa(bloqs[s]);
					string_append(&nombreBloque, numerito);
					string_append(&nombreBloque, ".bin");


					if(string_length(loQueVaQuedandoDeBuffer)>tamanioBloques){
						//cortar el string
						char* recortado=string_substring_until(loQueVaQuedandoDeBuffer, tamanioBloques);
						adx_store_data(nombreBloque,recortado);
						loQueVaQuedandoDeBuffer=string_substring_from(loQueVaQuedandoDeBuffer, tamanioBloques);

					}else{
						//mandarlo todo de una
						adx_store_data(nombreBloque,loQueVaQuedandoDeBuffer);
					}



					//actualizamos el bitmap
					bitarray_set_bit(bitarray,bloqs[s]);

				}




				//actualizamos el metadata del archivo con los nuevos bloques y el nuevo tamano del archivo
				FILE *fp = fopen(nombreArchivoRecibido, "w");//Para que borre todo lo que tenia antes
				char *dataAPonerEnFile = string_new();
				string_append(&dataAPonerEnFile, "TAMANIO=");
				char* tamanioArchivoViejo=obtTamanioArchivo(nombreArchivoRecibido);
				int tamanioArchivoViejoInt=atoi(tamanioArchivoViejo);
				int tamanioNuevo=tamanioArchivoViejoInt+(cuantosBloquesMasNecesito*tamanioBloques);
				char* tamanioNuevoChar=string_itoa(tamanioNuevo);
				string_append(&dataAPonerEnFile, tamanioNuevoChar);
				string_append(&dataAPonerEnFile,"\n");
				string_append(&dataAPonerEnFile, "BLOQUES=[");
				int z;

				   char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);
				   int d=0;
					   while(!(arrayBloques[d] == NULL)){
						   string_append(&dataAPonerEnFile,arrayBloques[d]);
						   string_append(&dataAPonerEnFile,",");
					      d++;
					   }
				for(z=0;z<cuantosBloquesMasNecesito;z++){
					char* bloqueString=string_itoa(bloqs[z]);
					string_append(&dataAPonerEnFile,bloqueString);
					if(!(z==(cuantosBloquesMasNecesito-1))){
						string_append(&dataAPonerEnFile,",");
					}
				}

				string_append(&dataAPonerEnFile,"]");

				adx_store_data(nombreArchivoRecibido,dataAPonerEnFile);

				validado=1;
				send(socket_cliente,&validado,sizeof(int),0);
				return;
				//y enviamos un buen send

			}else{
				validado=0;
				send(socket_cliente,&validado,sizeof(int),0);
				return;
				//send con error
			}



		}
	validado=1;
	send(socket_cliente,&validado,sizeof(int),0);
	}else{
		validado=0;
		send(socket_cliente,&validado,sizeof(int),0); //El archivo no existe
	}

}

