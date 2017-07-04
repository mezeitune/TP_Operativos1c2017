#include "sockets.h"
#include <commons/log.h>
#include <commons/collections/list.h>
#include "logger.h"
 #include <fcntl.h>

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

int getSizeBloque(FILE* bloque);
void actualizarMetadataArchivo(char* path,int size,t_list* nuevosBloques);

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
	   validado=0;
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

	log_info(loggerConPantalla,"Obteniendo datos--->Archivo:%s--->Posicion cursor:%d--->Size:%d",nombreArchivo,cursor,size);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);


	if( access(nombreArchivoRecibido, F_OK ) != -1 ) {


		fp = fopen(nombreArchivoRecibido, "rb");

		char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);

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

		   int d=0;
		   while(!(cursor%tamanioBloques)==cursor) d++; //Para saber cual es el primer bloque a leer

		   printf("El primer bloque a leer es :%d\n",d);
		   FILE *bloque;

		   int sizeRestante=size;


		   int sizeDentroBloque=0;
		   int cantidadBloquesLeidos=0;
		   while(cantidadBloquesLeidos < cantidadBloquesQueNecesito){
			   printf("Leyendo data del bloque:%s\n",arrayBloques[d]);

			   if(cantidadBloquesLeidos==0){
				   if(size>tamanioBloques-cursor) sizeDentroBloque=tamanioBloques-cursor;//Leo todo el bloque
				   else sizeDentroBloque = size; //Leo lo suficiente

				   char *nombreBloque = string_new();
				   string_append(&nombreBloque, puntoMontaje);
				   string_append(&nombreBloque, "Bloques/");
				   string_append(&nombreBloque, arrayBloques[d]);
				   string_append(&nombreBloque, ".bin");

				   bloque=fopen(nombreBloque, "rb");
				   void* data=obtenerBytesDeUnArchivo(bloque,cursor,sizeDentroBloque); //En el primero bloque, arranca del cursor
				   string_append(&infoTraidaDeLosArchivos,data);
				   sizeRestante -= sizeDentroBloque;
			   }
			   else{
				   if(sizeRestante < tamanioBloques) sizeDentroBloque = sizeRestante;
				   else sizeDentroBloque = tamanioBloques;

				   char *nombreBloque = string_new();
				   string_append(&nombreBloque, puntoMontaje);
				   string_append(&nombreBloque, "Bloques/");
				   string_append(&nombreBloque, arrayBloques[d]);
				   string_append(&nombreBloque, ".bin");

				   bloque=fopen(nombreBloque, "rb");
				   void* data=obtenerBytesDeUnArchivo(bloque,0,sizeDentroBloque); //Siempre arranca del principio
				   string_append(&infoTraidaDeLosArchivos,data);
				   sizeRestante -= sizeDentroBloque;
			   }
			   d++; //Avanzo de bloque
			   cantidadBloquesLeidos++;
		   }


/*			int hizoLoQueNecesita=0;
  			 int u=1;

		   while((arrayBloques[d]!=NULL)){
		   printf("Leyendo data del bloque:%s\n",arrayBloques[d]);
			   if(cursor<=(tamanioBloques*u)){

				   int t;
				   int inicial=d;
				   for(t=inicial;t<((inicial+cantidadBloquesQueNecesito));t++){ /*TODO:t<((inicial+cantidadBloquesQueNecesito)+1) Estaba asi, pero rompia mati*/
					/*   hizoLoQueNecesita=1;
					   int indice=atoi(arrayBloques[t]);

						char *nombreBloque = string_new();
						string_append(&nombreBloque, puntoMontaje);
						string_append(&nombreBloque, "Bloques/");
						string_append(&nombreBloque, arrayBloques[t]);
						string_append(&nombreBloque, ".bin");

						FILE *bloque=fopen(nombreBloque, "rb");

						if(t==(d+cantidadBloquesQueNecesito)){

							int sizeQuePido=size-cursor;
							int offsetQuePido=0;
							void* data=obtenerBytesDeUnArchivo(bloque,offsetQuePido,sizeQuePido); /*TODO: Te cambio para que leea el bloque, y no el archivo en si*/
						/*	string_append(&infoTraidaDeLosArchivos,data);

						}else if(t==inicial){

							int offsetQuePido=cursor-(tamanioBloques*u);
							int sizeQuePido=tamanioBloques-offsetQuePido;
							string_append(&infoTraidaDeLosArchivos,obtenerBytesDeUnArchivo(bloque,offsetQuePido , sizeQuePido));

						}else{

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
		 * */


		 printf("La info leida es:%s\n",infoTraidaDeLosArchivos);
		//si todod ok
		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
		send(socket_cliente,infoTraidaDeLosArchivos,size,0);
	} else {
		log_error(loggerConPantalla,"No se puede leer porque el archivo no existe");
		validado=-1;
		send(socket_cliente,&validado,sizeof(int),0); //El archivo no existe
	}


}




void guardarDatosArchivoFunction(int socket_cliente){//ver tema puntero, si lo tengo que recibir o que onda
	FILE* bloque;
	int tamanoNombreArchivo;
	int validado;
	int cursor;
	int size;

	int cuantosBloquesMasNecesito;
	t_list* nuevosBloques = list_create();

	recv(socket_cliente,&tamanoNombreArchivo,sizeof(int),0);
	printf("Tamano nombre archivo:%d\n",tamanoNombreArchivo);
	char* nombreArchivo = malloc(tamanoNombreArchivo + sizeof(char));

	recv(socket_cliente,nombreArchivo,tamanoNombreArchivo,0);
	strcpy(nombreArchivo + tamanoNombreArchivo, "\0");
	printf("Nombre archivo:%s\n",nombreArchivo);

	recv(socket_cliente,&cursor,sizeof(int),0);
	printf("Puntero:%d\n",cursor);

	recv(socket_cliente,&size,sizeof(int),0);
	printf("Tamano de la data:%d\n",size);
	void* buffer = malloc(size);

	recv(socket_cliente,buffer,size,0);
	printf("Data :%s\n",(char*)buffer);

	log_info(loggerConPantalla,"Guardando datos--->Archivo:%s--->Informacion:%s",nombreArchivo,(char*)buffer);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, nombreArchivo);

	printf("Toda la ruta :%s\n",nombreArchivoRecibido);

	if( access( nombreArchivoRecibido, F_OK ) != -1 ) {

		char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);

		int indiceBloque=0;
		int cantidadBloquesArchivo = 0;
		while(!(arrayBloques[indiceBloque] == NULL)){
			indiceBloque++;
			cantidadBloquesArchivo ++;
		}
		indiceBloque--; //Quiero el indice al ultimo bloque

		printf("Cantidad de bloques :%d\n",cantidadBloquesArchivo);

		char *direccionBloque = string_new();
		string_append(&direccionBloque, puntoMontaje);
		string_append(&direccionBloque, "Bloques/");
		string_append(&direccionBloque, arrayBloques[indiceBloque]);
		string_append(&direccionBloque, ".bin");


		printf("Direccion del ultimo bloque: %s\n",direccionBloque);
		//ver de asignar mas bloques en caso de ser necesario
		printf("Tamano del archivo : %d\n",atoi(obtTamanioArchivo(nombreArchivoRecibido)));
		printf("Tamano del bloque: %d\n",tamanioBloques);
		int cantidadRestanteUltimoBloque=tamanioBloques-(atoi(obtTamanioArchivo(nombreArchivoRecibido))-((cantidadBloquesArchivo-1)*tamanioBloques));
		printf("Cantidad restante en el ultimo bloque :%d\n",cantidadRestanteUltimoBloque);

		if(size<cantidadRestanteUltimoBloque){ //Con el primer bloque me alcanza

			bloque = fopen(direccionBloque,"ab");
			fseek(bloque,cursor,SEEK_SET);
			fwrite(buffer,size,1,bloque);
			log_info(loggerConPantalla,"Datos guardados--->Archivo:%s--->Informacion:%s",nombreArchivo,(char*)buffer);
			fclose(bloque);
			//adx_store_data(direccionBloque,buffer);

		}else{ //Necesito mas bloques

			 /*TODO: Emprolijar*/
		   if((size%tamanioBloques) == 0) cuantosBloquesMasNecesito = 1;
		   if((size%tamanioBloques) < size) cuantosBloquesMasNecesito = 1;
		   if((size%tamanioBloques) == size) {
			   cuantosBloquesMasNecesito = size / tamanioBloques ;
			   cuantosBloquesMasNecesito += 1;
		   }

/*
			int cuantosBloquesMasNecesito=size/tamanioBloques;

			if((size%tamanioBloques)>0){
				cuantosBloquesMasNecesito++;
			}
			*/

			printf("Bloques de mas que necesito:%d\n",cuantosBloquesMasNecesito);

			int numeroBloque;
			int bloquesEncontrados=0;

			for(numeroBloque=0;numeroBloque<cantidadBloques;numeroBloque++){
		        bool bit = bitarray_test_bit(bitarray,numeroBloque);
		        if(bit==0){
		        	bloquesEncontrados++;
		        	list_add(nuevosBloques,&numeroBloque);
		        }
		        if(bloquesEncontrados==cuantosBloquesMasNecesito) break;
			}

			printf("Bloques encontrados :%d\n",bloquesEncontrados); /*TODO: Cuando crea el archivo en el mismo proceso, y despues solicita esciribr, no encuentra bloques*/

			if(bloquesEncontrados>=cuantosBloquesMasNecesito){
				log_info(loggerConPantalla,"Existen bloques disponibles para almacenar la informacion");
				//guardamos en los bloques deseados

				int s;

				int sizeRestante = size;
				int desplazamiento = 0;

				//char* loQueVaQuedandoDeBuffer=(char*)buffer;/*TODO: OJO ACA, el size es 0. Deberias preguntar sobre el size a escribir, e ir actualizando eso*/

				for(s=0;s<cuantosBloquesMasNecesito;s++){
					char *nombreBloque = string_new();
					string_append(&nombreBloque, puntoMontaje);
					string_append(&nombreBloque, "Bloques/");
					string_append(&nombreBloque, string_itoa(*(int*)list_get(nuevosBloques,s)));
					string_append(&nombreBloque, ".bin");

					printf("Voy a guardar en el bloque:%s\n",string_itoa(*(int*)list_get(nuevosBloques,s)));
					bloque=fopen(nombreBloque,"ab");

					if(sizeRestante>tamanioBloques){ //if(string_length(loQueVaQuedandoDeBuffer)>tamanioBloques)
						printf("Tengo que cortar el string\n");
						//cortar el string
						fwrite(buffer + desplazamiento , tamanioBloques, 1,bloque);
						//char* recortado=string_substring_until(buffer + desplazamiento, tamanioBloques);
						//adx_store_data(nombreBloque,recortado);
						sizeRestante -= tamanioBloques;
						desplazamiento += tamanioBloques;
						//loQueVaQuedandoDeBuffer=string_substring_from(loQueVaQuedandoDeBuffer, tamanioBloques);

					}else{
						printf("No tuve que cortar el string\n");
						//mandarlo todo de una
						fwrite(buffer + desplazamiento , sizeRestante, 1,bloque);
						//adx_store_data(nombreBloque,buffer + desplazamiento);
					}
					fclose(bloque);

					//actualizamos el bitmap
					bitarray_set_bit(bitarray,*(int*)list_get(nuevosBloques,s));
				}


			}else{
				log_error(loggerConPantalla,"No existen suficientes bloques para escribir la informacion solicitada");
				validado=0;
				send(socket_cliente,&validado,sizeof(int),0);
				return;
			}
		}

		actualizarMetadataArchivo(nombreArchivoRecibido,size,nuevosBloques);
		validado=1;
		send(socket_cliente,&validado,sizeof(int),0);
	}else{
		log_error(loggerConPantalla,"El archivo no fue creado--->Archivo:%s",nombreArchivo);
		validado=0;
		send(socket_cliente,&validado,sizeof(int),0); //El archivo no existe
	}

}

void actualizarInformacionEnBloque(char* direccionBloque,char* buffer,int size){

	FILE* bloque=fopen(direccionBloque,"ab");
	fclose(bloque);

	bloque = fopen(direccionBloque,"rb");
	int sizeAntiguo;
	char* bufferAntiguo;
	char* bufferTotal;

	fseek(bloque,0,SEEK_END);
	sizeAntiguo = ftell(bloque);
	bufferAntiguo = malloc(sizeAntiguo);
	fseek(bloque,0,SEEK_SET);
	fread(bufferAntiguo,sizeof(char),sizeAntiguo,bloque);
	fclose(bloque);

	bloque = fopen(direccionBloque,"wb");

	bufferTotal = malloc(sizeAntiguo + size);
	strcat(bufferTotal,bufferAntiguo);
	strcat(bufferTotal,buffer);

	fwrite(bufferTotal,sizeof(char),sizeAntiguo+size,bloque);
	fclose(bloque);

}

void actualizarMetadataArchivo(char* path,int size,t_list* nuevosBloques){
	log_info(loggerConPantalla,"Actualizando metadata de archivo:%s",path);
	int i;
			//Obtenemos los datos viejos
			int tamanioArchivoViejo=atoi(obtTamanioArchivo(path));
			char**arrayBloquesViejos=obtArrayDeBloquesDeArchivo(path);

			int tamanioNuevo= tamanioArchivoViejo + size;

			 //actualizamos el metadata del archivo con los nuevos bloques y el nuevo tamano del archivo
			FILE *fp = fopen(path, "w");//Para que borre todo lo que tenia antes
			char *metadataFile = string_new();

			string_append(&metadataFile, "TAMANIO=");
			string_append(&metadataFile, string_itoa(tamanioNuevo));
			string_append(&metadataFile,"\n");

			string_append(&metadataFile, "BLOQUES=[");

			int indiceBloque=0;
		   while(!(arrayBloquesViejos[indiceBloque] == NULL)){
			   string_append(&metadataFile,arrayBloquesViejos[indiceBloque]);
			   if(arrayBloquesViejos[indiceBloque+1]!=NULL)string_append(&metadataFile,",");
			   indiceBloque++;
		   }

			for(i=0;i<nuevosBloques->elements_count;i++){
				string_append(&metadataFile,",");
				char* bloqueString=string_itoa(*(int*)list_get(nuevosBloques,i));//string_itoa(bloqs[z])
				string_append(&metadataFile,bloqueString);
			}

			string_append(&metadataFile,"]");
			fclose(fp); //Lo cierro porque la proxima linea lo volvia a abrir
			adx_store_data(path,metadataFile);
}
