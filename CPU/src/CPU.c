#include "CPU.h"

void handShakeKernel(int socketKernel);

int paginaSize;

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logCPU.txt");
	listaPcb = list_create();
	log_info(loggerConPantalla, "Inicia proceso CPU");

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);


	//char comandoGetNuevoProceso = 'N';
	//handShakeKernel(socketKernel);
	//send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);

	//Lo primero que habria que hacer en realidad es aca pedirle un PCB al kernel
	//a travez de serializacion y que este me envie uno si es que tiene programas pendientes de CPU
	//en caso de no tener ningun programa pendiente , simplemente se imprime un mensaje
	//Cuando ya tengo ese PCB (yo diria que establecido globalmente en CPU)
	//Le pido info a memoria desplazandome en cuanto a sus instrucciones y ejecutando las primitivas necesarias
	//para responderle al kernel todos los resultados y que se manden a consola

	signal(SIGUSR1, signalSigusrHandler);//Para mandar la signal:
				//htop->pararse sobre el proceso a mandarle signal->k->SIGUSR1->ENTER
	comenzarEjecucionNuevoPrograma();
	return 0;
}

void handShakeKernel(int socketKernel){
	char comandoGetPaginaSize= 'P';
	send(socketKernel,&comandoGetPaginaSize,sizeof(char),0);
	recv(socketKernel,&paginaSize,sizeof(int),0);
}
void comenzarEjecucionNuevoPrograma(){
	//while(counterPCBAsignado==0){//loopear hasta conseguir PCB(queda en espera activa)
		char comandoRecibirPCB;
		char comandoGetNuevoProceso = 'N';
		send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);

	//}


		log_info(loggerConPantalla, "Ejecutando");

		//char *programa = strdup(PROGRAMA);//copia el programa entero en esa variable
		//t_metadata_program *metadata = metadata_desde_literal(programa);//hacerlo por que si
		//int programCounter = 0;//deberia ser el del PCB
		/*while(!terminoElPrograma()){
			char* const linea = conseguirDatosDeLaMemoria(programa,
			metadata->instrucciones_serializado[programCounter].start,
			metadata->instrucciones_serializado[programCounter].offset);//que me devuelva la siguiente linea la memoria
			printf("\t Evaluando -> %s", linea);
			analizadorLinea(linea, &functions, &kernel_functions);//que haga lo que tenga q hacer
			log_info(loggerConPantalla, "CPU lee una linea");
			free(linea);
			programCounter++;
		}*/
		//metadata_destruir(metadata);//por que si
		printf("================\n");

		recv(socketKernel,&comandoRecibirPCB,sizeof(char),0);
		connectionHandlerKernel(socketKernel,comandoRecibirPCB);
}

void connectionHandlerKernel(int socketAceptado, char orden) {


	//if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	switch (orden) {
		case 'S':
				recibirPCByEstablecerloGlobalmente(socketAceptado);
					break;
		default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
	}
orden = '\0';
return;
}

void recibirPCByEstablecerloGlobalmente(int socketKernel){

	//counterPCBAsignado++;

	pcbAUtilizar *pcb = malloc(sizeof(pcbAUtilizar));
	recibirYDeserializarPcb(socketKernel);

	log_info(loggerConPantalla, "CPU recibe PCB correctamente");


	list_add(listaPcb, pcb);


	//y setea counterPCBnoASignado en 0
	//counterPCBAsignado=0;
	//si no recibio PCB correcto , incremendo counterPCBnoASignado

	//if(counterPCBAsignado==0){//PCBcorrecto?
		//puse 1 para que no de syntax error momentaneamente
		//printf("Ya hay un PCB asignado a esta CPU");
	//}


	//if(counterPCBAsignado==1){
		//log_warning(loggerConPantalla, "Todavia no hay ningun PCB para asignar a esta CPU");
	//}

}



//char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i){
//}
void conseguirDatosMemoria (pcbAUtilizar* pcb, int paginaSolicitada, int size){
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina
	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	//send(socketMemoria,&pcb->offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	char* mensajeRecibido = recibir_string(socketMemoria);
	log_info(loggerConPantalla,"\nEl mensaje recibido de la Memoria es : %s\n" , mensajeRecibido);

}

int pedirBytesYAlmacenarEnMemoria(pcbAUtilizar *pcb){
	int puedeAlmacenarDatosEnMemoria = pedirBytesMemoria(pcb);

	if (puedeAlmacenarDatosEnMemoria==1){
		int size = strlen("bonebone");
		printf("puede almacenar\n");
		almacenarDatosEnMemoria(pcb,"bonebone",size);

			return 1;
		}
	return 0;

}

int pedirBytesMemoria(pcbAUtilizar* pcb){

		int resultadoEjecucion=1;
		char comandoAsignacion = 'G';
		int paginaAPedir=2;

		send(socketMemoria,&comandoAsignacion,sizeof(char),0);
		send(socketMemoria,&pcb->pid,sizeof(int),0);
		send(socketMemoria,&paginaAPedir,sizeof(int),0);
		//recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		printf("Se pidio bytes correctamente\n");

		return resultadoEjecucion;
}

int almacenarDatosEnMemoria(pcbAUtilizar* pcb,char* buffer, int size){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';

		int paginaDondeGuardoDatos = 1; // valor arbitrario

		send(socketMemoria,&comandoAlmacenar,sizeof(char),0);
		send(socketMemoria,&pcb->pid,sizeof(int),0);
		send(socketMemoria,&paginaDondeGuardoDatos,sizeof(int),0);
		//send(socketMemoria,&pcb->offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);

		printf("Se almaceno correctamente %s",buffer);
		return resultadoEjecucion;

}



void interfazHandler(int socket){

	char orden;

	pcbAUtilizar *pcb = malloc(sizeof(pcbAUtilizar));
	pcb = list_get(listaPcb,0);
	int size = strlen("bonebone");
	while(1){
		while(orden != 'Q'){

			printf("\nIngresar orden:\n");
			scanf(" %c", &orden);

			switch(orden){
				case 'S':

					conseguirDatosMemoria(pcb,0,size);//el 0 es la pagina a la cual buscar y el size es de la instruccion
					break;
				case 'A':
					pedirBytesYAlmacenarEnMemoria(pcb);
					break;

				case 'F':
					finalizar();
					pcbAUtilizar *pcbAEliminar = list_get(listaPcb,0);


						//asigno pcb en memoria
					 //pcbAEliminar->pid;

					log_info(loggerConPantalla,"\nSea finalizado el programa con pid%s\n" , pcbAEliminar->pid);
					break;
				case 'Q':
					log_warning(loggerConPantalla,"\nSe ha desconectado el cliente\n");
					exit(1);
					break;
				default:
					log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
					break;
			}
		}
			orden = '\0';
	}
}


void finalizar (){

	//comando para  avisarle al kernel que debe eliminar
	char comandoInicializacion = 'F';

	void* pcbAEliminar= malloc(sizeof(char) + sizeof(int) * 2); //pido memoria para el comando que deba usar el kernel + los 2 int de la estructura del pcb

	//agarro el pcb que quiero eliminar
	pcbAUtilizar *unPcbAEliminar = list_get(listaPcb,0);

	//pido memoria para ese pcb
	pcbAUtilizar *infoPcbAEliminar = malloc(sizeof(pcbAUtilizar));

	//asigno pcb en memoria
	infoPcbAEliminar->pid = unPcbAEliminar->pid;
	//infoPcbAEliminar->cantidadPaginas = unPcbAEliminar->cantidadPaginas;

	//printf("%d%d",unPcbAEliminar->pid,unPcbAEliminar->cantidadPaginas);
	//serializarPCByEnviar(socketKernel,comandoInicializacion,&infoPcbAEliminar,pcbAEliminar);

	list_destroy_and_destroy_elements(listaPcb, free);

	counterPCBAsignado=0;
	//un send avisandole al kernel q termino ese proceso con su header
	free(pcbAEliminar);

}

pcbAUtilizar* recibirYDeserializarPcb(int socketKernel){
	log_info(loggerConPantalla, "Recibiendo PCB serializado---- SOCKET:%d", socketKernel);
	int pcbSerializadoSize;
	recv(socketKernel,&pcbSerializadoSize,sizeof(int),0);
	printf("pasa");
	void * pcbSerializado = malloc(pcbSerializadoSize);
	recv(socketKernel,&pcbSerializado,pcbSerializadoSize,0);
	pcbAUtilizar* pcb = malloc(sizeof(pcbAUtilizar));

	memcpy(&pcb->pid,pcbSerializado,sizeof(int));
	log_info(loggerConPantalla, "Deserializando PCB ----- PID:%d",pcb->pid);
	memcpy(&pcb->cantidadPaginasCodigo,pcbSerializado + sizeof(int),sizeof(int));
	memcpy(&pcb->programCounter, pcbSerializado + sizeof(int)*2, sizeof(int));
	memcpy(&pcb->cantidadInstrucciones, pcbSerializado + sizeof(int)*3, sizeof(t_puntero_instruccion));
	int indiceCodigoSize = calcularIndiceCodigoSize(pcb->cantidadInstrucciones);
	memcpy(&pcb->indiceCodigo,pcbSerializado + sizeof(int)*3, indiceCodigoSize);
	memcpy(&pcb->cantidadEtiquetas,pcbSerializado + sizeof (int) *3 + indiceCodigoSize, sizeof(int));
	int indiceEtiquetasSize=calcularIndiceEtiquetasSize(pcb->cantidadEtiquetas);
	memcpy(&pcb->indiceEtiquetas, pcbSerializado + sizeof(int)*4 + indiceCodigoSize, indiceEtiquetasSize);
	pcb->indiceStack=list_create();
	deserializarStack(pcbSerializado + sizeof(int)*4 + indiceCodigoSize,&pcb->indiceStack);

	return pcb;
}

void deserializarStack(void* pcbSerializado, t_list** indiceStack){
	log_info(loggerConPantalla, "Deserializando Stack");
		int i,j;
		t_nodoStack* node;
		t_posMemoria* argumento;
		t_variable* variable;

		int cantidadArgumentos;
		int cantidadVariables;
		int cantidadElementosStack;

		memcpy(&cantidadElementosStack,(int*) pcbSerializado,sizeof(int));
		pcbSerializado += sizeof(int);

		for(i = 0; i < cantidadElementosStack;i++){
			node = malloc(sizeof(t_nodoStack));
			node->args = list_create();
			memcpy(&cantidadArgumentos,(int*) pcbSerializado, sizeof(int));
			pcbSerializado += sizeof(int);

			for(j = 0; j < cantidadArgumentos; j++){
				argumento = malloc(sizeof(t_posMemoria));
				memcpy(&argumento,(t_posMemoria*) pcbSerializado,sizeof(t_posMemoria));
				list_add(node->args,argumento);
				pcbSerializado += sizeof(t_posMemoria);
			}

		memcpy(&cantidadVariables,(int*)pcbSerializado,sizeof(int));
			pcbSerializado += sizeof(int);

			node->vars = list_create();

			for(j = 0; j < cantidadVariables; j++){
				variable = malloc(sizeof(t_variable));
				memcpy(&variable->idVar,(char*) pcbSerializado,sizeof(char));
				pcbSerializado += sizeof(char);
				variable->dirVar = malloc(sizeof(t_posMemoria));
				memcpy(&variable->dirVar,(t_posMemoria*) pcbSerializado,sizeof(t_posMemoria));
				pcbSerializado += sizeof(t_posMemoria);
				list_add(node->vars, variable);
			}

		memcpy(&node->retPos, (int*) pcbSerializado,sizeof(int));
			pcbSerializado += sizeof(int);
			node->retVar=malloc(sizeof(t_posMemoria));
		memcpy(&node->retVar,(t_posMemoria*) pcbSerializado,sizeof(t_posMemoria));
			pcbSerializado += sizeof(t_posMemoria);
			list_add(*indiceStack, node);
		}

}
int calcularIndiceCodigoSize(int cantidadInstrucciones){
	log_info(loggerConPantalla, "Calculando tamano del Indice de Codigo");
	return cantidadInstrucciones * sizeof(int) * 2;
}

int calcularIndiceEtiquetasSize(int cantidadEtiquetas){
	log_info(loggerConPantalla, "Calculando tamano del Indice de Etiquetas");
	return cantidadEtiquetas*sizeof(char);
}
void signalSigusrHandler(int signum)
{
    if (signum == SIGUSR1)
    {
        //printf("Received SIGUSR1!\n");

    	//hacer un send a memoria para avisar que se desconecto la CPU y que no se ponga como loca
		log_warning(loggerConPantalla,"\nSe ha desconectado CPU con signal SIGUSR1\n");
		exit(1);
    }
}
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden) {
		log_info(loggerConPantalla,"\n--Esperando una orden del cliente %d-- \n", socketCliente);
		//recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
		log_info(loggerConPantalla,"El cliente %d ha enviado la orden: %c\n", socketCliente, nuevaOrden);
}

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



