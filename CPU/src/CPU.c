#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
//#include <parser/metadata_program.h>
#include <commons/log.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
//#include <parser/metadata_program.h>
#include <commons/log.h>
#include <pthread.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "conexiones.h"
#include "dummy_ansisop.h"
#include <parser/metadata_program.h>
static const char* PROGRAMA =
						"begin\n"
						"variables a, b\n"
						"a = 3\n"
						"b = 5\n"
						"a = b + 12\n"
						"end\n"
						"\n";
typedef struct PCB {
	int pid;
	int cantidadPaginas;
	//int* programCounter;
	//int** indiceCodigo;
	int offset;
	//Indice del Stack
	//int exitCode;
}pcbAUtilizar;

AnSISOP_funciones functions = {  //TODAS LAS PRIMITIVAS TIENEN QUE ESTAR ACA
	.AnSISOP_definirVariable	= dummy_definirVariable,
	.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
	.AnSISOP_finalizar = dummy_finalizar,
	.AnSISOP_dereferenciar	= dummy_dereferenciar,
	.AnSISOP_asignar	= dummy_asignar,
	/*
	 .AnSISOP_obtenerValorCompartida
	 .AnSISOP_asignarValorCompartida
	 .AnSISOP_irAlLabel
	 .AnSISOP_llamarSinRetorno
	 .AnSISOP_llamarConRetorno
	 .AnSISOP_retornar
	 */
};

AnSISOP_kernel kernel_functions = {/*
		.AnSISOP_wait
		.AnSISOP_signal
		.AnSISOP_reservar
		.AnSISOP_liberar
		.AnSISOP_abrir
		.AnSISOP_borrar
		.AnSISOP_cerrar
		.AnSISOP_moverCursor
		.AnSISOP_escribir
		.AnSISOP_leer
		*/

};//NO SE PARA QUE ES ESTO
char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i);

//-----------------------------------------------------------------------------------------------------------------

void recibirPCByEstablecerloGlobalmente(int socketKernel);//Falta implementar , y que reciba con serializacion desde Kernel
									//En caso de que el PCB se haya seteado en 0 , deberia quedar a la espera
									//de nuevos PCB hasta poder ejecutar algo
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socketKernel);
void* conexionMemoria (int socketMemoria);

void comenzarEjecucionNuevoPrograma();
void signalSigusrHandler(int signum);

//utilizacion de la memoria
int pedirBytesMemoria(pcbAUtilizar* pcb);
int almacenarDatosEnMemoria(pcbAUtilizar* pcb,char* buffer, int size);
int pedirBytesYAlmacenarEnMemoria();
void conseguirDatosMemoria (pcbAUtilizar* pcb, int paginaSolicitada, int size);
//utilizacion de la memoria


t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;


//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//



//------------------Sockets Globales-------//
int socketMemoria;
int socketKernel;
//-----------------------------------------//



t_list* listaPcb;


pthread_t HiloConexionMemoria;


void serializarPCByEnviar(int socket, char comandoInicializacion, pcbAUtilizar *unPcbAEliminar, void* pcbAEliminar);
void finalizar();

int counterPCBAsignado=0;//Cuando esto incremente a 1 , significa que ya recibio un PCB correcto
					//si queda en 0 significa que no hay todavia. Cuando la CPU se libere del PCB actual porque
					//ya realizo todas sus operaciones correspondientes , entonces se vuelve a setear en 0


void cargarPcbActual(pcbAUtilizar* pidEstructura, int pid, int cantidadPaginas, int offset) {
	pidEstructura->pid=pid;
	pidEstructura->cantidadPaginas =cantidadPaginas;
	pidEstructura->offset= offset;
}




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
	while(counterPCBAsignado==0){//loopear hasta conseguir PCB(queda en espera activa)
		recibirPCByEstablecerloGlobalmente(socketKernel);
	}


	log_info(loggerConPantalla, "Ejecutando");

		char *programa = strdup(PROGRAMA);//copia el programa entero en esa variable
		t_metadata_program *metadata = metadata_desde_literal(programa);//hacerlo por que si
		int programCounter = 0;//deberia ser el del PCB
		while(!terminoElPrograma()){
			char* const linea = conseguirDatosDeLaMemoria(programa,
			metadata->instrucciones_serializado[programCounter].start,
			metadata->instrucciones_serializado[programCounter].offset);//que me devuelva la siguiente linea la memoria
			printf("\t Evaluando -> %s", linea);
			analizadorLinea(linea, &functions, &kernel_functions);//que haga lo que tenga q hacer
			log_info(loggerConPantalla, "CPU lee una linea");
			free(linea);
			programCounter++;
		}
		metadata_destruir(metadata);//por que si
		printf("================\n");



	//conseguirDatosMemoria(pcb,0,8);
		connectionHandler(socketKernel);
		}



char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i){
}
void conseguirDatosMemoria (pcbAUtilizar* pcb, int paginaSolicitada, int size){
	//char comandoSolicitud= 'S';
	char * mensajeRecibido;
	//send(socketMemoria,&comandoSolicitud,sizeof(char),0);
	printf("bone");
	send(socketMemoria,&pcb->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&pcb->cantidadPaginas,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	mensajeRecibido = recibir_string(socketMemoria);
	printf("%s",mensajeRecibido);
}





int pedirBytesYAlmacenarEnMemoria(){

	pcbAUtilizar* pcb = malloc (sizeof(pcbAUtilizar));
	int puedeAlmacenarDatosEnMemoria = pedirBytesMemoria(pcb);

		if (puedeAlmacenarDatosEnMemoria){

			almacenarDatosEnMemoria(pcb,"bonebone",8);
			return 1;
		}
	return 0;
	free(pcb);
}

int pedirBytesMemoria(pcbAUtilizar* pcb){

		void* mensajeAMemoria = malloc(sizeof(int)*2);
		int resultadoEjecucion=1;
		//char comandoInicializacion = 'A';

		//memcpy(mensajeAMemoria,&comandoInicializacion,sizeof(char));
		memcpy(mensajeAMemoria, &pcb->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(int) , &pcb->cantidadPaginas , sizeof(int));

		send(socketMemoria,mensajeAMemoria,sizeof(int)*2,0);
		printf("puede pasar ");
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);


		free(mensajeAMemoria);
		return resultadoEjecucion;
}

int almacenarDatosEnMemoria(pcbAUtilizar* pcb,char* buffer, int size){
		int resultadoEjecucion=1;
		//int comandoAlmacenar = 'C';

		int paginaSolicitada = 0; // valor arbitrario

		//void * mensajeAMemoria= malloc(sizeof(char) + sizeof(int)* 4 + size);
		//memcpy(mensajeAMemoria,&comandoAlmacenar,sizeof(char));
		//memcpy(mensajeAMemoria ,&pcb->pid,sizeof(int));
		//memcpy(mensajeAMemoria + sizeof(int),&paginaSolicitada,sizeof(int));
		//memcpy(mensajeAMemoria + sizeof(int)*2,&pcb->offset,sizeof(int));
		//memcpy(mensajeAMemoria + sizeof(int)*3,&size,sizeof(int));
		//memcpy(mensajeAMemoria + sizeof(int)*4,buffer,size);
		printf("puede");
		send(socketMemoria,&pcb->pid,sizeof(int),0);
		send(socketMemoria,&paginaSolicitada,sizeof(int),0);
		send(socketMemoria,&pcb->offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);


		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
		return resultadoEjecucion;
		//free(mensajeAMemoria);
}






void recibirPCByEstablecerloGlobalmente(socketKernel){


	//logica de serializacion para recibir PCB
	//si recibio un PCB , se guarda en la estructura
	counterPCBAsignado++;
	pcbAUtilizar* pcbNuevo = malloc(sizeof(pcbAUtilizar));
	log_info(loggerConPantalla, "CPU recibe PCB correctamente");
	cargarPcbActual(pcbNuevo,1,2,1);
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
	free (pcbNuevo);
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


	char* mensajeRecibido;

	int paginaAPedir=0;
	int offset=0;
	int pid=1;
	int size=46;

	char comandoSolicitar = 'S';

	while(1){
		while(orden != 'Q'){

			printf("\nIngresar orden:\n");
			scanf(" %c", &orden);

			switch(orden){
				case 'S':
					send(socketMemoria,&comandoSolicitar,sizeof(char),0);
					send(socketMemoria,&pid,sizeof(int),0);
					send(socketMemoria,&paginaAPedir,sizeof(int),0);
					send(socketMemoria,&offset,sizeof(int),0);
					send(socketMemoria,&size,sizeof(int),0);

					mensajeRecibido = recibir_string(socketMemoria);
					log_info(loggerConPantalla,"\nEl mensaje recibido de la Memoria es : %s\n" , mensajeRecibido);

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





