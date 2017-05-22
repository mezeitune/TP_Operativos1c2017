#include "CPU.h"


int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");
	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	//send(socketKernel,'N',sizeof(char),0);


	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);


	log_info(loggerConPantalla, "Inicia proceso CPU");
	recibirTamanioPagina(socketKernel);
	listaPcb = list_create();
	signal(SIGUSR1, signalSigusrHandler);


	if(cpuOcupada == 1){
			recibirPCB();
			cpuOcupada--;
	}else
		log_warning(loggerConPantalla, "No se le asigno un PCB a esta CPU");

	return 0;
}

void recibirPCB(){


		char comandoRecibirPCB;
		char comandoGetNuevoProceso = 'N';
		send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);
		recv(socketKernel,&comandoRecibirPCB,sizeof(char),0);
		log_info(loggerConPantalla, "Se ha avisado que se quiere enviar un PCB...\n");
		connectionHandlerKernel(socketKernel,comandoRecibirPCB);

}

void connectionHandlerKernel(int socketAceptado, char orden) {

	if(orden == '\0')nuevaOrdenDeAccion(socketAceptado, orden);

	switch (orden) {
		case 'S':
			log_info(loggerConPantalla, "Se esta por asignar un PCB");
				establecerPCB(socketAceptado);
					break;
		default:
				if(orden == '\0') break;
				log_warning(loggerConPantalla,"\nOrden %c no definida\n", orden);
				break;
	}
orden = '\0';
return;
}

void establecerPCB(){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb = recibirYDeserializarPcb(socketKernel);

	log_info(loggerConPantalla, "CPU recibe PCB correctamente\n");


	list_add(listaPcb, pcb);

	ciclosDeQuantum(pcb);
	interfazHandler(pcb);//esto despues se saca, es para probar funciones nomas

}
void ciclosDeQuantum(t_pcb* pcb){
	int i = 0;
	int quantum_definido=1;
	//recv(socketKernel,&quantum_definido,sizeof(int),0);
	while((i < quantum_definido)){
		ejecutarInstruccion(pcb);
		i++;
	}
	//if((i == quantum_definido)){
		//send(socketKernel,&comandoTerminoElQuantum , sizeof(char),0);
	//}
}
void ejecutarInstruccion(t_pcb* pcb){

	char* instruccion = obtener_instruccion(pcb);
	printf("Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);//que haga lo que tenga q hacer
	free(instruccion);
	pcb->programCounter = pcb->programCounter + 1;

}

int conseguirDatosMemoria (char** instruccion, t_pcb* pcb, int paginaSolicitada,int offset,int size){
	int resultadoEjecucion;
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina
	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*instruccion = recibir_string(socketMemoria);
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

int almacenarDatosEnMemoria(t_pcb* pcb,char* buffer, int size){
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
void interfazHandler(t_pcb* pcb){

	char orden;
	int size = strlen("bonebone");
	char* instruccion;

	while(1){
		while(orden != 'Q'){

			printf("\nIngresar orden:\n");
			scanf(" %c", &orden);

			switch(orden){
				case 'S':

					//instruccion = conseguirDatosMemoria(pcb,0,6,16);//el 0 es la pagina a la cual buscar y el size es de la instruccion

					break;

				case 'F':
					finalizar();

					log_info(loggerConPantalla,"\nSea finalizado el programa con pid%s\n" , pcb->pid);
					free(pcb);
					break;
				case 'Q':
					log_warning(loggerConPantalla,"\nSe ha desconectado el cliente\n");
					free(pcb);
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

char* obtener_instruccion(t_pcb * pcb){
	int program_counter = pcb->programCounter;
	int byte_inicio_instruccion = pcb->indiceCodigo[program_counter][0];
	int bytes_tamanio_instruccion = pcb->indiceCodigo[program_counter][1];

	int num_pagina = byte_inicio_instruccion / paginaSize;
	int offset = byte_inicio_instruccion - (num_pagina * paginaSize);//no es 0 porque evita el begin
	char* mensajeRecibido;
	char* instruccion;
	char* continuacion_instruccion;
	int bytes_a_leer_primera_pagina;

	if (bytes_tamanio_instruccion > (paginaSize * 2)){
		printf("El tamanio de la instruccion es mayor al tamanio de pagina\n");
	}
	if ((offset + bytes_tamanio_instruccion) < paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_tamanio_instruccion)<0)
			printf("No se pudo solicitar el contenido\n");
		else
			instruccion=mensajeRecibido;
	} else {
		bytes_a_leer_primera_pagina = paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					printf("No se pudo solicitar el contenido\n");
		else
					instruccion=mensajeRecibido;

		log_info(loggerConPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido,pcb, (num_pagina + 1),offset,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
								printf("No se pudo solicitar el contenido\n");
					else
								continuacion_instruccion=mensajeRecibido;
			log_info(loggerConPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(loggerConPantalla, "La continuacion de la instruccion es 0. Ni la leo");
		}
	}
	char** string_cortado = string_split(instruccion, "\n");
	free(instruccion);
	instruccion = string_new();
	string_append(&instruccion, string_cortado[0]);
	log_info(loggerConPantalla, "\nInstruccion obtenida: %s", instruccion);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return instruccion;
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
		recv(socketCliente, &nuevaOrden, sizeof nuevaOrden, 0);
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

//--------------------------------------------PRIMITIVAS-------------------------------------------------


t_puntero definirVariable(t_nombre_variable variable) {
	printf("pasa");

	t_pcb *pcb_actual = malloc(sizeof(t_pcb));
	pcb_actual = list_get(listaPcb,0);
	int nodos_stack = list_size(pcb_actual->indiceStack);
	int posicion_stack;
	int cantidad_variables;
	int cantidad_argumentos;
	int encontre_valor = 1;
	t_nodoStack *nodo;
	t_posMemoria *posicion_memoria;
	t_variable *var;
	t_posMemoria* nueva_posicion_memoria;
	t_variable *nueva_variable;
	//if(pcb_finalizar == 1){
		//return 0;
	//}
	if(nodos_stack == 0){ //si el stack esta vacio
		t_nodoStack *nodo = malloc(sizeof(t_nodoStack));//creo un nodo con todos los campos
		nodo->args = list_create();//creo la lista de argumentos y las variables para ese nodo vacio
		nodo->vars = list_create();
		nodo->retPos = 0;//cuando arranca el programa el retorno es 0
		t_posMemoria *retorno = malloc(sizeof(t_posMemoria));
		retorno->pagina = 0;//en memoria escribe en la pagina 0 del stack
		retorno->offset = 0;
		retorno->size = 0;
		nodo->retVar = retorno;//asigno a la estructura retVar la posicion en memoria del retorno
		list_add(pcb_actual->indiceStack, nodo);//agrego un nodo al stack
	}
	//ya habiendo agregado el nodo o si tengo un stack con nodos anteriores
	nodos_stack = list_size(pcb_actual->indiceStack);
	//hago un for recorriendo los nodos que esten en el stack
	for(posicion_stack = (nodos_stack - 1); posicion_stack >= 0; posicion_stack--){
			nodo = list_get(pcb_actual->indiceStack, posicion_stack); //agarro un nodo de mi stack
			cantidad_variables = list_size(nodo->vars);//agarro su lista de variables y arg
			cantidad_argumentos = list_size(nodo->args);
			if(cantidad_variables != 0){//si este nodo tiene variables...
				var = list_get(nodo->vars, (cantidad_variables - 1));//agarro una variable del nodo
				posicion_memoria = var->dirVar; //agarro la posicion en memoria de esa variable (pag,off,size), aca no esta el id de la variable
				posicion_stack = -1;
				encontre_valor = 0;//encontre una variablee!!!
			} else if(cantidad_argumentos != 0){//si este nodo tiene argumentos
				posicion_memoria = list_get(nodo->args, (cantidad_argumentos - 1));//agarro la pos memoria de ese argumento (pag,off,size)
				posicion_stack = -1;
				encontre_valor = 0;//encontre un argumentoooooooooo!!
			}
		}
		//reconozco la variable ANSISOP
		if((variable >= '0') && (variable <= '9')){
				nueva_posicion_memoria = malloc(sizeof(t_posMemoria));//creo una nueva posicion en memoria para la variable ANSISOP
				nodo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));//agarro un nodo de mi stack (el indice es la cantidad-1)

				if(encontre_valor == 0){//si encontre una variable en el nodo

					//si me excedi del tamaño de la pagina
					if(((posicion_memoria->offset + posicion_memoria->size) + 4) > paginaSize){
						nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);//le digo de escribir en la pagina sig
						nueva_posicion_memoria->offset = 0;
						nueva_posicion_memoria->size = 4;
							if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){//si la pagina excede la cantidad de pagina del stack
								stackOverflow(pcb_actual);
							} else {
							list_add(nodo->args, nueva_posicion_memoria);//sino agrego en la lista de argumentos la posicion en memoria de esa variaable
							}
					}
					//si me excedi del tamaño de la pagina

					else {//sino me excedi del tamaño de la pagina
									nueva_posicion_memoria->pagina = posicion_memoria->pagina;
									nueva_posicion_memoria->offset = (posicion_memoria->offset + posicion_memoria->size);
									nueva_posicion_memoria->size = 4;
									if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){
										stackOverflow(pcb_actual);
									} else {
										list_add(nodo->args, nueva_posicion_memoria);
									}
					}//sino me excedi del tamaño de la pagina
				}else {
							if(paginaSize < 4){
								printf("Tamaño de pagina menor a 4 bytes\n");
							} else {
								//le asigno la pagina donde empieza el stack (ver)
								nueva_posicion_memoria->pagina = (pcb_actual->cantidadPaginasCodigo - stackSize);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){
									stackOverflow(pcb_actual);
								} else {
									list_add(nodo->args, nueva_posicion_memoria);
								}
							}
						}
		}//si mi variable no esta entre 0 y 9
				else {
						nueva_posicion_memoria = malloc(sizeof(t_posMemoria));
						nueva_variable = malloc(sizeof(t_variable));
						nodo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));
						if(encontre_valor == 0){
							if(((posicion_memoria->offset + posicion_memoria->size) + 4) > paginaSize){//si me paso de pagina
								nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								nueva_variable->idVar = variable;
								nueva_variable->dirVar = nueva_posicion_memoria;
								if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){
									stackOverflow(pcb_actual);
								} else {
									list_add(nodo->vars, nueva_variable);
								}
							}else {
								//sino me paso de pagina
								nueva_posicion_memoria->pagina = posicion_memoria->pagina;
								nueva_posicion_memoria->offset = (posicion_memoria->offset + posicion_memoria->size);
								nueva_posicion_memoria->size = 4;
								nueva_variable->idVar = variable;
								nueva_variable->dirVar = nueva_posicion_memoria;
								if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){
									stackOverflow(pcb_actual);
								} else {
									list_add(nodo->vars, nueva_variable);
								}
							}
						//sino encontre valor en la memoria
				}	else {
								if(paginaSize < 4){
									printf("Tamaño de pagina menor a 4 bytes\n");
									} else {
										nueva_posicion_memoria->pagina = (pcb_actual->cantidadPaginasCodigo - stackSize);//ACA MUESTRA -1 PORQUE HAY UNA PAGINA DE CODIGO Y 2 DE STACK
										nueva_posicion_memoria->offset = 0;
										nueva_posicion_memoria->size = 4;
										nueva_variable->idVar = variable;
										nueva_variable->dirVar = nueva_posicion_memoria;
										if(nueva_posicion_memoria->pagina >= pcb_actual->cantidadPaginasCodigo){
											stackOverflow(pcb_actual);
										} else {
											list_add(nodo->vars, nueva_variable);
										}
									}
								}
					}
imprimirPcb(pcb_actual);
int posicion= (nueva_posicion_memoria->pagina * paginaSize) + nueva_posicion_memoria->offset;

return posicion;
}
	void finalizar (){

		//comando para  avisarle al kernel que debe eliminar
		char comandoInicializacion = 'F';

		void* pcbAEliminar= malloc(sizeof(char) + sizeof(int) * 2); //pido memoria para el comando que deba usar el kernel + los 2 int de la estructura del pcb

		//agarro el pcb que quiero eliminar
		t_pcb *unPcbAEliminar = list_get(listaPcb,0);

		//pido memoria para ese pcb
		t_pcb *infoPcbAEliminar = malloc(sizeof(t_pcb));

		//asigno pcb en memoria
		infoPcbAEliminar->pid = unPcbAEliminar->pid;
		//infoPcbAEliminar->cantidadPaginasCodigo = unPcbAEliminar->cantidadPaginasCodigo;

		//printf("%d%d",unPcbAEliminar->pid,unPcbAEliminar->cantidadPaginasCodigo);
		//serializarPCByEnviar(socketKernel,comandoInicializacion,&infoPcbAEliminar,pcbAEliminar);

		list_destroy_and_destroy_elements(listaPcb, free);


		//un send avisandole al kernel q termino ese proceso con su header
		free(pcbAEliminar);

}
void stackOverflow(t_pcb* pcb_actual){
		log_info(loggerConPantalla, "El pid %d sufrio stack overflow", pcb_actual->pid);
		dummy_finalizar();

}
