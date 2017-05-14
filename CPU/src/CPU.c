#include "CPU.h"

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logCPU.txt");
	listaPcb = list_create();
	log_info(loggerConPantalla, "Inicia proceso CPU");

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);



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




void comenzarEjecucionNuevoPrograma(){
	//while(counterPCBAsignado==0){//loopear hasta conseguir PCB(queda en espera activa)
		recibirPCByEstablecerloGlobalmente(socketKernel);
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


		connectionHandler(socketKernel);
		}



//

//char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i){
//}
void conseguirDatosMemoria (pcbAUtilizar* pcb, int paginaSolicitada, int size){
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina
	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&pcb->offset,sizeof(int),0);
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
		send(socketMemoria,&pcb->offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);

		printf("Se almaceno correctamente %s",buffer);
		return resultadoEjecucion;

}

void recibirPCByEstablecerloGlobalmente(socketKernel){
	//logica de serializacion para recibir PCB
	//si recibio un PCB , se guarda en la estructura
	counterPCBAsignado++;
	pcbAUtilizar* pcbNuevo = malloc(sizeof(pcbAUtilizar));
	log_info(loggerConPantalla, "CPU recibe PCB correctamente");
	cargarPcbActual(pcbNuevo,1,0,0);
	list_add(listaPcb, pcbNuevo);


	//y setea counterPCBnoASignado en 0
	//counterPCBAsignado=0;
	//si no recibio PCB correcto , incremendo counterPCBnoASignado

	if(counterPCBAsignado==0){//PCBcorrecto?
		//puse 1 para que no de syntax error momentaneamente
		//printf("Ya hay un PCB asignado a esta CPU");
	}


	if(counterPCBAsignado==1){
		log_warning(loggerConPantalla, "Todavia no hay ningun PCB para asignar a esta CPU");
	}

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



void connectionHandler(int socket){

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
	infoPcbAEliminar->cantidadPaginas = unPcbAEliminar->cantidadPaginas;

	printf("%d%d",unPcbAEliminar->pid,unPcbAEliminar->cantidadPaginas);
	serializarPCByEnviar(socketKernel,comandoInicializacion,&infoPcbAEliminar,pcbAEliminar);

	list_destroy_and_destroy_elements(listaPcb, free);

	counterPCBAsignado=0;
	//un send avisandole al kernel q termino ese proceso con su header
	free(pcbAEliminar);

}



void serializarPCByEnviar(int socket, char comandoInicializacion, pcbAUtilizar *unPcb, void* pcb){
	//aca voy copiando en pcbAEliminar cada cosa que quiero empaquetar memcpy(a donde lo pongo con su posicion, que garcha pongo, tamaño de la garcha que pongo);
	memcpy(pcb,&comandoInicializacion,sizeof(char));
	memcpy(pcb + sizeof(char), &unPcb->pid,sizeof(int));
	memcpy(pcb + sizeof(char) + sizeof(int) , &unPcb->cantidadPaginas , sizeof(int));

	//envio al kernel lo empaquetado con su tamaño que hice previamente en el malloc
	send(socket, pcb, sizeof(int)*2 + sizeof(char), 0);


}

pcbAUtilizar* deserializarPCB(void* pcb_serializado){
	pcbAUtilizar* pcb = malloc(sizeof(pcbAUtilizar));
	pcb_serializado += sizeof(char);
	//evito el char si es que el kernel manda alguno antes
	memcpy(&pcb->pid, pcb_serializado, sizeof(int));
	pcb_serializado += sizeof(int);
	memcpy(&pcb->cantidadPaginas, pcb_serializado, sizeof(int));
	pcb_serializado += sizeof(int);


	return pcb;
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
void cargarPcbActual(pcbAUtilizar* pidEstructura, int pid, int cantidadPaginas, int offset) {
	pidEstructura->pid=pid;
	pidEstructura->cantidadPaginas =cantidadPaginas;
	pidEstructura->offset= offset;
}




