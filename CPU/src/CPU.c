#include "CPU.h"

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");
	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);


	log_info(loggerConPantalla, "Inicia proceso CPU");
	recibirTamanioPagina(socketKernel);

	signal(SIGINT, signalHandler);
	signal(SIGUSR1, signalHandler);


	char comandoGetNuevoProceso = 'N';
	send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);

	esperarPCB();

	return 0;
}
void esperarPCB(){

				listaPcb = list_create();
				while(cpuOcupada==1){

					log_warning(loggerConPantalla, "No se le asigno un PCB a esta CPU");
					recibirPCB();
					cpuOcupada--;

				}

}
void recibirPCB(){
		char comandoRecibirPCB;
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
int cantidadPaginasTotales(t_pcb* pcb){
	int paginasTotales= (stackSize + pcb->cantidadPaginasCodigo);
	return paginasTotales;
}
void establecerPCB(){

	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb = recibirYDeserializarPcb(socketKernel);

	log_info(loggerConPantalla, "CPU recibe PCB correctamente\n");


	list_add(listaPcb, pcb);
	printf("\n\nPCB:%d\n\n", pcb->pid);
	EjecutarProgramaMedianteAlgoritmo(pcb);


}
void EjecutarProgramaMedianteAlgoritmo(t_pcb* pcb){
	int i;
			i = 0;
	//recv(socketKernel,&ultimaInstruccion,sizeof(int),0);
	int cantidadInstruccionesAEjecutar=pcb->cantidadInstrucciones;

	while((i < cantidadInstruccionesAEjecutar)){
		ejecutarInstruccion(pcb);
		i++;
	}
	//if(i==(cantidadInstruccionesAEjecutar-1))
	//expropiarPorQuantum(pcb);
}
void ejecutarInstruccion(t_pcb* pcb){

	char* instruccion = obtener_instruccion(pcb);
	printf("Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);
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
	*instruccion=malloc((size+1)*sizeof(char));
	recv(socketMemoria,*instruccion,size,0);
	strcpy(*instruccion+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

int almacenarDatosEnMemoria(t_pcb* pcb,char* buffer, int size,int paginaAGuardar,int offset){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';
		send(socketMemoria,&comandoAlmacenar,sizeof(char),0);
		send(socketMemoria,&pcb->pid,sizeof(int),0);
		send(socketMemoria,&paginaAGuardar,sizeof(int),0);
		send(socketMemoria,&offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		return resultadoEjecucion;

}

char* obtener_instruccion(t_pcb * pcb){
	int program_counter = pcb->programCounter;
	int byte_inicio_instruccion = pcb->indiceCodigo[program_counter][0];
	int bytes_tamanio_instruccion = pcb->indiceCodigo[program_counter][1];
	int num_pagina = byte_inicio_instruccion / config_paginaSize;
	int offset = byte_inicio_instruccion - (num_pagina * config_paginaSize);//no es 0 porque evita el begin
	char* mensajeRecibido;
	char* mensajeRecibido2;
	char* instruccion;
	char* continuacion_instruccion;
	int bytes_a_leer_primera_pagina;

	if (bytes_tamanio_instruccion > (config_paginaSize * 2)){
		printf("El tamanio de la instruccion es mayor al tamanio de pagina\n");
	}
	if ((offset + bytes_tamanio_instruccion) < config_paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_tamanio_instruccion)<0)
			{
			printf("No se pudo solicitar el contenido\n");
			}
			else{
				instruccion=mensajeRecibido;
				}
	} else {
		bytes_a_leer_primera_pagina = config_paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					{printf("No se pudo solicitar el contenido\n");}
		else{
				instruccion=mensajeRecibido;
			}
		//free(mensajeRecibido);
		log_info(loggerConPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido2,pcb, (num_pagina + 1),0,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
					{printf("No se pudo solicitar el contenido\n");}
						else{
						continuacion_instruccion=mensajeRecibido2;
						log_info(loggerConPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
								}

			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(loggerConPantalla, "La continuacion de la instruccion es 0. Ni la leo");
		}
	}
	char** string_cortado = string_split(instruccion, "\n");
	free(instruccion);
	instruccion= string_new();
	string_append(&instruccion, string_cortado[0]);
	log_info(loggerConPantalla, "\nInstruccion obtenida: %s", instruccion);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	//	imprimirPcb(pcb);
	return instruccion;
}


void signalHandler(int signum)
{
    if (signum == SIGUSR1 || signum == SIGINT)
    {
    	log_warning(loggerConPantalla,"Cierre por signal, ejecutando ultimas instrucciones del pcb actual y cerrando CPU ...");
    	cpuFinalizada=0;
    }
}
void CerrarPorSignal(){
	char comandoInterruptHandler='X';
	char comandoCierreCpu='C';

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoCierreCpu,sizeof(char),0);
	 //hacer un send a memoria para avisar que se desconecto la CPU y que no se ponga como loca
	log_warning(loggerConPantalla,"\nSe ha desconectado CPU con signal correctamente\n");
	exit(1);
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
void expropiarPorQuantum(t_pcb * pcb){
	//char comandoTerminoElQuantum= 'R';
	//send(socketKernel,&comandoTerminoElQuantum , sizeof(char),0);
	serializarPcbYEnviar(pcb,socketKernel);
	log_info(loggerConPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por fin de quantum", pcb->pid);
	//list_destroy_and_destroy_elements(listaPcb, free);
	cpuOcupada=1;
}
void stackOverflow(t_pcb* pcb_actual){
		log_warning(loggerConPantalla, "Elproceso ANSISOP de PID %d sufrio stack overflow", pcb_actual->pid);
		finalizar();

}


//--------------------------------------------PRIMITIVAS-------------------------------------------------


t_puntero definirVariable(t_nombre_variable variable) {
	//t_pcb* pcb_actual = malloc(sizeof(t_pcb));
	t_pcb*pcb_actual = list_get (listaPcb,0);
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
					if(((posicion_memoria->offset + posicion_memoria->size) + 4) > config_paginaSize){
						nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);//le digo de escribir en la pagina sig
						nueva_posicion_memoria->offset = 0;
						nueva_posicion_memoria->size = 4;
							if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){//si la pagina excede la cantidad de paginas totales
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
									if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
										stackOverflow(pcb_actual);
									} else {
										list_add(nodo->args, nueva_posicion_memoria);
									}
					}//sino me excedi del tamaño de la pagina
					//sino encontre una variable, creo una nueva :)
				}else {
							if(config_paginaSize < 4){
								printf("Tamaño de pagina menor a 4 bytes\n");
							} else {
								//le asigno la pagina donde empieza el stack (ver)
								nueva_posicion_memoria->pagina = (cantidadPaginasTotales(pcb_actual) - stackSize);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
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
							if(((posicion_memoria->offset + posicion_memoria->size) + 4) > config_paginaSize){//si me paso de pagina
								nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								nueva_variable->idVar = variable;
								nueva_variable->dirVar = nueva_posicion_memoria;
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
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
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
									stackOverflow(pcb_actual);
								} else {
									list_add(nodo->vars, nueva_variable);
								}
							}
						//sino encontre valor en la memoria
				}	else {
								if(config_paginaSize < 4){
									printf("Tamaño de pagina menor a 4 bytes\n");
									} else {
										nueva_posicion_memoria->pagina = (cantidadPaginasTotales(pcb_actual) - stackSize);//ACA MUESTRA -1 PORQUE HAY UNA PAGINA DE CODIGO Y 2 DE STACK
										nueva_posicion_memoria->offset = 0;
										nueva_posicion_memoria->size = 4;
										nueva_variable->idVar = variable;
										nueva_variable->dirVar = nueva_posicion_memoria;
										if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
											stackOverflow(pcb_actual);
										} else {
											list_add(nodo->vars, nueva_variable);
										}
									}
								}
					}

int posicion= (nueva_posicion_memoria->pagina * config_paginaSize) + nueva_posicion_memoria->offset;

return posicion;
}




t_puntero obtenerPosicionVariable(t_nombre_variable variable) {
	log_info(loggerConPantalla, "Obteniendo la posicion de la variable: %c", variable);
	 t_pcb* pcb_actual = list_get (listaPcb,0);
	int nodos_stack = list_size(pcb_actual->indiceStack);//obtengo cantidad de nodos
	int cantidad_variables;
	int i;
	int encontre_valor = 1;
	t_nodoStack *nodoUltimo;
	t_posMemoria *posicion_memoria;
	//t_posMemoria* nueva_posicion_memoria;
//	t_variable *nueva_variable;
	t_variable *var;
	nodoUltimo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));//obtengo el ultimo nodo de la lista
	if((variable >= '0') && (variable <= '9')){//si esta entre 0 y 9 significa que es un argumento de una funcion
		int variable_int = variable - '0';//lo pasa a int

		posicion_memoria = list_get(nodoUltimo->args, variable_int);//lo busca en la lista de argumentos
		if(posicion_memoria != NULL){
			encontre_valor = 0;
		}
	} else {//si es una variable propiamente dicha
		cantidad_variables = list_size(nodoUltimo->vars);
		for(i = 0; i < cantidad_variables; i++){
			var = list_get(nodoUltimo->vars, i);
			if(var->idVar == variable){
				posicion_memoria = var->dirVar;
				encontre_valor = 0;
			}
		}
	}
	if(encontre_valor == 1){
		log_info(loggerConPantalla, "ObtenerPosicionVariable: No se encontro variable o argumento\n");
		return -1;
	}
	int posicion_serializada = (posicion_memoria->pagina * config_paginaSize) + posicion_memoria->offset;//me devuelve la posicion en memoria
	//free(pcb_actual);

	return posicion_serializada;
}
void finalizar (){
		t_pcb * pcb_actual = list_get(listaPcb,0);
		char comandoFinalizacion = 'T';
		send(socketKernel,&comandoFinalizacion,sizeof(char),0);



		serializarPcbYEnviar(pcb_actual,socketKernel);





		log_info(loggerConPantalla, "El proceso ANSISOP de PID %d ha finalizado", pcb_actual->pid);
		list_destroy_and_destroy_elements(listaPcb, free);
		if(cpuFinalizada==0){
		CerrarPorSignal();
		}
		cpuOcupada=1;
		esperarPCB();
}

t_valor_variable dereferenciar(t_puntero puntero) {
	int num_pagina = puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);
	char *valor_variable_char;
	char* mensajeRecibido;
	t_pcb* pcb_actual = malloc(sizeof(t_pcb));
	pcb_actual = list_get (listaPcb,0);
		if ( conseguirDatosMemoria(&mensajeRecibido,pcb_actual, num_pagina,offset, 4)<0){
				printf("No se pudo solicitar el contenido\n");
		}else{
				valor_variable_char=mensajeRecibido;
		}
	log_info(loggerConPantalla, "Valor Obtenido de la Memoria: %s", valor_variable_char);
	char *ptr;
	int valor_variable = strtol(valor_variable_char, &ptr, 10);
	free(valor_variable_char);
	log_info(loggerConPantalla, "dereferenciar: Valor Obtenido: %d", valor_variable);
	return valor_variable;

}

void retornar(t_valor_variable retorno){
	t_pcb *pcb_actual = list_get(listaPcb,0);
	t_nodoStack *nodo;
	int cantidad_nodos = list_size(pcb_actual->indiceStack);
	nodo = list_remove(pcb_actual->indiceStack, (cantidad_nodos - 1));
	t_posMemoria *posicion_memoria;
	posicion_memoria = nodo->retVar;
	int num_pagina = posicion_memoria->pagina;
	int offset = posicion_memoria->offset;
	char *valor_variable = string_itoa(retorno);
	almacenarDatosEnMemoria(pcb_actual,valor_variable,4,num_pagina, offset);
	free(valor_variable);
	//pcb_actual->programCounter = nodo->retPos;// Puede ser la dir_retorno + 1
	//Elimino el nodo de la lista
	int cantidad_argumentos;
	int cantidad_variables;
	t_variable *var;
	cantidad_argumentos = list_size(nodo->args);
	while(cantidad_argumentos != 0){
		posicion_memoria = list_remove(nodo->args, (cantidad_argumentos - 1));
		free(posicion_memoria);
		cantidad_argumentos = list_size(nodo->args);
	}
	list_destroy(nodo->args);
	cantidad_variables = list_size(nodo->vars);
	while(cantidad_variables != 0){
		var = list_remove(nodo->vars, (cantidad_variables - 1));
		free(var->dirVar);
		free(var);
		cantidad_variables = list_size(nodo->vars);
	}
	list_destroy(nodo->vars);
	free(nodo->retVar);
	free(nodo);
	imprimirPcb(pcb_actual);

}


void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
t_pcb *pcb_actual = list_get(listaPcb,0);
t_nodoStack *nodo = malloc(sizeof(t_nodoStack));
nodo->args = list_create();
nodo->vars = list_create();
nodo->retPos = (pcb_actual->programCounter);//Puede ser programCounter + 1
int num_pagina = donde_retornar / config_paginaSize;
int offset = donde_retornar - (num_pagina * config_paginaSize);
t_posMemoria *retorno = malloc(sizeof(t_posMemoria));
retorno->pagina = num_pagina;
retorno->offset = offset;
retorno->size = 4;
nodo->retVar = retorno;
list_add(pcb_actual->indiceStack, nodo);
char** string_cortado = string_split(etiqueta, "\n");
int program_counter = metadata_buscar_etiqueta(string_cortado[0], pcb_actual->indiceEtiquetas, pcb_actual->indiceEtiquetasSize);
	if(program_counter == -1){
		printf("No se encontro la funcion %s en el indice de etiquetas\n", string_cortado[0]);
	} else {
		pcb_actual->programCounter = (program_counter - 1);
	}
int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
free(string_cortado);

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

}

void irAlLabel(t_nombre_etiqueta etiqueta){

t_pcb * pcb_actual = list_get(listaPcb,0);
char** string_cortado = string_split(etiqueta, "\n");
int program_counter = metadata_buscar_etiqueta(string_cortado[0], pcb_actual->indiceEtiquetas, pcb_actual->indiceEtiquetasSize);
	if(program_counter == -1){
		log_info(loggerConPantalla, "No se encontro la etiqueta: %s en el indice de etiquetas", string_cortado[0]);
	} else {
		pcb_actual->programCounter = (program_counter - 1);
		log_info(loggerConPantalla, "Program Counter, despues de etiqueta: %d", pcb_actual->programCounter);
	}
int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
free(string_cortado);

}

void asignar(t_puntero puntero, t_valor_variable variable) {

	t_pcb *pcb_actual = list_get (listaPcb,0);
	int num_pagina = puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);
	char *valor_variable = string_itoa(variable);
	almacenarDatosEnMemoria(pcb_actual,valor_variable,4,num_pagina, offset);
	log_info(loggerConPantalla, "asignar: Valor a Asignar: %s", valor_variable);
	free(valor_variable);
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	void* variable_a_enviar;
	int tamanio;
	//send(socketKernel,variable_a_enviar,tamanio,0);

	free(variable_string);
	free(variable_a_enviar);

	int* valor_variable_recibida = malloc(sizeof(int));
	//valor_variable_recibida = (int*) recv(socketKernel,,,);
	int valor_variable = *valor_variable_recibida;
	free(valor_variable_recibida);
	log_info(loggerConPantalla, "Valor de la variable compartida: %d", valor_variable);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor_variable;
}
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	void* variable_a_enviar;
	//int tamanio = calcularTamanio(variable_string, valor, &variable_serializada);
	//send(socketKernel,variable,tamanio,0);
	free(variable_string);
	free(variable_a_enviar);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor;
}

//-------------------------------------------Kernel primitivas------------------------------------------------//


void wait(t_nombre_semaforo identificador_semaforo){

	t_pcb* pcb_actual = list_get (listaPcb,0);
	char** string_cortado = string_split(identificador_semaforo, "\n");
	log_info(loggerConPantalla, "Semaforo a bajar: %s", string_cortado[0]);
	//void* wait_serializado;
	//int tamanioMensaje = serializarWait(string_cortado[0], &wait_serializado);
	//send(socketKernel,&wait_serializado,tamanioMensaje,0);
	//free(wait_serializado);
	char* mensaje = recibir_string(socketKernel);
	if(strcmp(mensaje, "dale para adelante!") != 0){
		//pcb_bloqueado = 1;
		log_info(loggerConPantalla, "pid: %d bloqueado por semaforo: %s", pcb_actual->pid, string_cortado[0]);
	}
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	free(mensaje);

}
void signal_Ansisop(t_nombre_semaforo identificador_semaforo){
	char** string_cortado = string_split(identificador_semaforo, "\n");
	log_info(loggerConPantalla, "Semaforo a subir: %s", string_cortado[0]);
	void* signal;
	int tamanio;
	//send(socketKernel,signal,tamanio,0);
	free(signal);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
}

t_puntero reservar (t_valor_variable espacio){
	char comandoInterruptHandler = 'X';
	char comandoReservarMemoria = 'R';
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoReservarMemoria,sizeof(char),0);
	int tamanio = sizeof(t_valor_variable);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,&espacio,tamanio,0);
	int* puntero;
	//recibir el puntero del kernel donde almaceno ese espacio en memoria
	//guardar ese puntero en una lista o estructura para hacer la comprobacion en liberar
	return puntero;
}
void liberar (t_puntero puntero){
	//list_find(listaPunterosHeap,puntero);
	//if(list_size(listaPunterosHeap)){
	char comandoInterruptHandler = 'X';
	char comandoLiberarMemoria = 'L';
	int resultadoEjecucion;
	int tamanio = sizeof(t_puntero);
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoLiberarMemoria,sizeof(char),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,&puntero,tamanio,0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(loggerConPantalla,"Se ha liberado correctamente el heap previamente reservado apuntando a %d",puntero);
	else
		log_info(loggerConPantalla,"No se ha podido liberar el heap apuntada por",puntero);
	//}
	//else
	log_info(loggerConPantalla,"Primero se debe reservar para liberar el heap");
}


t_descriptor_archivo abrir_archivo(t_direccion_archivo direccion, t_banderas flags){
	t_pcb* pcb_actual = list_get (listaPcb,0);
	t_descriptor_archivo descriptorArchivoAbierto;
	char comandoCapaFS = 'F';
	char comandoAbrirArchivo = 'A';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoAbrirArchivo,sizeof(char),0);
	send(socketKernel,pcb_actual->pid,sizeof(int),0);
	send(socketKernel,&direccion,sizeof(int),0);
	//enviar los flags al kernel
	recv(socketKernel,&descriptorArchivoAbierto,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1){
		log_info(loggerConPantalla,"El proceso de PID %d ha abierto un archivo de descriptor %d en modo %s");
		return descriptorArchivoAbierto;
	}
	else {
		log_info(loggerConPantalla,"Error del proceso de PID %d al abrir un archivo de descriptor %d en modo %s");
		return 0;
	}
}
void borrar_archivo (t_descriptor_archivo descriptor_archivo){
	t_pcb* pcb_actual = list_get (listaPcb,0);
	char comandoCapaFS = 'F';
	char comandoBorrarArchivo = 'B';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoBorrarArchivo,sizeof(char),0);
	send(socketKernel,pcb_actual->pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(loggerConPantalla,"El proceso de PID %d ha borrado un archivo de descriptor %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d al borrar el archivo de descriptor %d");
}

void cerrar_archivo(t_descriptor_archivo descriptor_archivo){
	t_pcb* pcb_actual = list_get (listaPcb,0);
	char comandoCapaFS = 'F';
	char comandoCerrarArchivo = 'P';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoCerrarArchivo,sizeof(char),0);
	send(socketKernel,pcb_actual->pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
	log_info(loggerConPantalla,"El proceso de PID %d ha cerrado un archivo de descriptor %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d ha cerrado el archivo de descriptor %d");

}


void moverCursor_archivo (t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	t_pcb* pcb_actual = list_get (listaPcb,0);
	char comandoCapaFS = 'F';
	char comandoMoverCursorArchivo = 'M';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoMoverCursorArchivo,sizeof(char),0);
	send(socketKernel,pcb_actual->pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&posicion,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
	log_info(loggerConPantalla,"El proceso de PID %d ha movido el cursor de un archivo de descriptor %d en la posicion %d");
	else log_info(loggerConPantalla,"Error del proceso de PID %d al mover el cursor de un archivo de descriptor %d en la posicion %d");
}
void leer_archivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	t_pcb* pcb_actual = list_get (listaPcb,0);
	char comandoCapaFS = 'F';
	char comandoLeerArchivo = 'O';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoLeerArchivo,sizeof(char),0);
	send(socketKernel,pcb_actual->pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&informacion,sizeof(int),0); //puntero que apunta a la direccion donde quiero obtener la informacion
	send(socketKernel,&tamanio,sizeof(int),0); //tamanio de la instruccion en bytes que quiero leer
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
	log_info(loggerConPantalla,"La informacion leida es %s");
	else log_info(loggerConPantalla,"Error del proceso de PID %d al leer informacion de un archivo de descriptor %d en la posicion %d");
}


void escribir(t_descriptor_archivo descriptor_archivo, t_valor_variable valor, t_valor_variable tamanio){
	if(descriptor_archivo==DESCRIPTOR_SALIDA){
		t_pcb* pcb_actual = list_get (listaPcb,0);
		char *valor_variable = string_itoa(valor);
		char comandoImprimir = 'X';
		char comandoImprimirPorConsola = 'P';
		send(socketKernel,&comandoImprimir,sizeof(char),0);
		send(socketKernel,&comandoImprimirPorConsola,sizeof(char),0);
		send(socketKernel,&tamanio,sizeof(t_valor_variable),0);
		send(socketKernel,&valor_variable,tamanio,0);
		send(socketKernel,&pcb_actual->pid,sizeof(int),0);
	}else {
		t_pcb* pcb_actual = list_get (listaPcb,0);
			char comandoCapaFS = 'F';
			char comandoEscribirArchivo = 'E';
			int resultadoEjecucion ;
			send(socketKernel,&comandoCapaFS,sizeof(char),0);
			send(socketKernel,&comandoEscribirArchivo,sizeof(char),0);
			send(socketKernel,pcb_actual->pid,sizeof(int),0);
			send(socketKernel,&descriptor_archivo,sizeof(int),0);
			//send(socketKernel,&valor,sizeof(int),0); //puntero que apunta a la direccion donde quiero obtener la informacion
			send(socketKernel,&tamanio,sizeof(int),0);
			recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
			if(resultadoEjecucion==1)
			log_info(loggerConPantalla,"La informacion ha sido escrita con exito en el archivo de descriptor %d PID %d");
			else log_info(loggerConPantalla,"Error del proceso de PID %d al escribir un archivo de descriptor %d ");
	}
}


