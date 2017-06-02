/*
 * conexionMemoria.h
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#ifndef CONEXIONMEMORIA_H_
#define CONEXIONMEMORIA_H_

int socketMemoria;
int solicitarContenidoAMemoria(char** mensajeRecibido);
int pedirMemoria(t_pcb* procesoListo);
int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado, char* programa, int programSize);
int calcularTamanioParticion(int *programSizeRestante);
void handshakeMemoria();




void handshakeMemoria(){
	char comandoTamanioPagina = 'P';
	send(socketMemoria,&comandoTamanioPagina,sizeof(char),0);
	recv(socketMemoria,&config_paginaSize,sizeof(int),0);
}


int pedirMemoria(t_pcb* procesoListo){
	log_info(loggerConPantalla, "Solicitando Memoria ---- PID: %d", procesoListo->pid);
		void* mensajeAMemoria = malloc(sizeof(int)*2 + sizeof(char));
		int paginasTotalesRequeridas = procesoListo->cantidadPaginasCodigo + stackSize;
		int resultadoEjecucion=1;
		char comandoInicializacion = 'A';

		memcpy(mensajeAMemoria,&comandoInicializacion,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char), &procesoListo->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(char) + sizeof(int) , &paginasTotalesRequeridas, sizeof(int));
		send(socketMemoria,mensajeAMemoria,sizeof(int)*2 + sizeof(char),0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		free(mensajeAMemoria);
		return resultadoEjecucion;
}

int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado,char* programa, int programSize){
	log_info(loggerConPantalla, "Almacenando programa en memoria ---- PID: %d", procesoListoAutorizado->pid);
		char* mensajeAMemoria = malloc(sizeof(char) + sizeof(int)* 4 + config_paginaSize);
		char* particionCodigo = malloc(config_paginaSize);
		int particionSize;
		int programSizeRestante = programSize;
		int resultadoEjecucion=0;
		int comandoAlmacenar = 'C';
		int offset=0;
		int nroPagina;

		for(nroPagina=0; nroPagina<procesoListoAutorizado->cantidadPaginasCodigo && resultadoEjecucion==0;nroPagina++){
				particionSize=calcularTamanioParticion(&programSizeRestante);
			//	log_info(loggerConPantalla, "Tamano de la particion de codigo a almacenar:\n %d\n", particionSize);
				strncpy(particionCodigo,programa,particionSize);
				strcpy(particionCodigo + particionSize,"\0");
				programa += particionSize;

				//log_info(loggerConPantalla, "Particion de codigo a almacenar: \n%s", particionCodigo);

				memcpy(mensajeAMemoria,&comandoAlmacenar,sizeof(char));
				memcpy(mensajeAMemoria + sizeof(char),&procesoListoAutorizado->pid,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)+sizeof(char),&nroPagina,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)*2 + sizeof(char),&offset,sizeof(int));

				memcpy(mensajeAMemoria + sizeof(int)*3 + sizeof(char),&particionSize,sizeof(int));
				memcpy(mensajeAMemoria + sizeof(int)*4 + sizeof(char),particionCodigo,particionSize);
				send(socketMemoria,mensajeAMemoria,sizeof(char) + sizeof(int)* 4 + particionSize,0);

				recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		}
		//log_info(loggerConPantalla, "Programa almacenado en Memoria---- PID: %d", procesoListoAutorizado->pid);
		free(mensajeAMemoria);
		free(particionCodigo);

		return resultadoEjecucion;

}
int calcularTamanioParticion(int *programSizeRestante){
		int mod=*programSizeRestante % config_paginaSize;
				 if(mod == *programSizeRestante){
					return *programSizeRestante;
			 }
				else{
					*programSizeRestante -= config_paginaSize;
					return config_paginaSize;
			 }
}

int solicitarContenidoAMemoria(char ** mensajeRecibido){
	log_info(loggerConPantalla, "Solicitando contenido a Memoria");
	char comandoSolicitud= 'S';
	int pid;
	int paginaSolicitada;
	int offset;
	int size;
	int resultadoEjecucion;
	printf("Se inicializa una peticion de consulta\n");
	send(socketMemoria,&comandoSolicitud,sizeof(char),0);
	printf("Ingrese el pid del proceso solicitado\n");
	scanf("%d",&pid);
	printf("Ingrese la pagina solicitada\n");
	scanf("%d",&paginaSolicitada);
	printf("Ingrese el offset \n");
	scanf("%d",&offset);
	printf("Ingrese el tamano del proceso\n");
	scanf("%d",&size);

	send(socketMemoria,&pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*mensajeRecibido = malloc((size + 1 )*sizeof(char));
	recv(socketMemoria,*mensajeRecibido,size,0);
	strcpy(*mensajeRecibido+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}




#endif /* CONEXIONMEMORIA_H_ */
