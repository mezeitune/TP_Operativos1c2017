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
#include <commons/log.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "conexiones.h"
#include "dummy_ansisop.h"
#include <parser/metadata_program.h>

typedef struct{
	int pagina;
	int offset;
	int size;
}__attribute__((packed)) t_posMemoria;


typedef struct{
	char idVar;
	t_posMemoria* dirVar;
} t_variable;

typedef struct{
	t_list* args; //Lista de argumentos. Cada posicion representa un argumento en el orden de la lista
	t_list* vars; // Lista de t_variable
	int retPos;
	t_posMemoria* retVar;
} t_nodoStack;


typedef struct {
		int pid;
		t_puntero_instruccion programCounter;
		int cantidadPaginasCodigo;
		int cantidadInstrucciones;
		int** indiceCodigo;
		int cantidadEtiquetas;
		char* indiceEtiquetas;
		t_list* indiceStack;
		int exitCode;
	}t_pcb;
void inicializarLog(char *rutaDeLog);
t_log *loggerSinPantalla;
t_log *loggerConPantalla;

int calcularIndiceCodigoSize(int cantidadInstrucciones);
int calcularIndiceEtiquetasSize(int cantidadEtiquetas);
int calcularIndiceStackSize(t_list* indiceStack);
void serializarStack(void** pcbSerializado,t_list* indiceStack);
void deserializarStack(void* pcbSerializado,t_list** indiceStack);
int calcularPcbSerializadoSize(t_pcb* pcb);
void serializarPcbYEnviar(t_pcb* pcb,int socketCPU);
t_pcb* recibirYDeserializarPcb(int socketKernel);

void serializarPcbYEnviar(t_pcb* pcb,int socketCPU){
	log_info(loggerConPantalla, "Serializando PCB ----- PID:%d",pcb->pid);

	int pcbSerializadoSize = calcularPcbSerializadoSize(pcb);
	int indiceEtiquetasSize=calcularIndiceEtiquetasSize(pcb->cantidadEtiquetas);
	int indiceCodigoSize=calcularIndiceCodigoSize(pcb->cantidadInstrucciones);
	int indiceStackSize=calcularIndiceStackSize(pcb->indiceStack);
	void* pcbSerializado= malloc(pcbSerializadoSize);

	memcpy(pcbSerializado,&pcb->pid,sizeof(int));
	memcpy(pcbSerializado + sizeof(int), &pcb->cantidadPaginasCodigo, sizeof(int));
	memcpy(pcbSerializado + sizeof(int)*2, &pcb->programCounter, sizeof(t_puntero_instruccion));
	memcpy(pcbSerializado + sizeof(int)*2 + sizeof(t_puntero_instruccion), &pcb->cantidadInstrucciones,sizeof(int));
	memcpy(pcbSerializado + sizeof(int)*3 + sizeof(t_puntero_instruccion), &pcb->indiceCodigo,indiceCodigoSize);
	memcpy(pcbSerializado + sizeof(int)*3 + sizeof(t_puntero_instruccion)+ indiceCodigoSize, &pcb->cantidadEtiquetas,sizeof(int));
	memcpy(pcbSerializado + sizeof(int)*4 + sizeof(t_puntero_instruccion) + indiceCodigoSize , &pcb->indiceEtiquetas, indiceEtiquetasSize);
	serializarStack(&pcbSerializado + sizeof(int)*4 + indiceCodigoSize + indiceEtiquetasSize, pcb->indiceStack);
	memcpy(pcbSerializado + sizeof(int)*4 + sizeof(t_puntero_instruccion) + indiceCodigoSize + indiceStackSize,&pcb->exitCode,sizeof(int));


	log_info(loggerConPantalla, "Enviando PCB serializado ----- PID: %d ------ socketCPU: %d", pcb->pid, socketCPU);
		printf("Pid: %d\n", pcb->pid);
		printf("Program Counter: %d\n", pcb->programCounter);

		printf("Cantidad de Instrucciones: %d\n", pcb->cantidadInstrucciones);

		printf("\n-------Indice de Codigo-------\n");
		int i;
		for(i = 0; i < pcb->cantidadInstrucciones; i++){
			printf("Instruccion %d:  \t%d %d\n", i, pcb->indiceCodigo[i][0], pcb->indiceCodigo[i][1]);
		}

		printf("\n-------Indice de Etiquetas-------\n");
		printf("%s\n", pcb->indiceEtiquetas);

	send(socketCPU,&pcbSerializadoSize,sizeof(int),0);
	send(socketCPU,pcbSerializado,pcbSerializadoSize,0);

}


t_pcb* recibirYDeserializarPcb(int socketKernel){
	log_info(loggerConPantalla, "Recibiendo PCB serializado---- SOCKET:%d", socketKernel);
	int pcbSerializadoSize;
	recv(socketKernel,&pcbSerializadoSize,sizeof(int),0);

	void * pcbSerializado = malloc(pcbSerializadoSize);
	recv(socketKernel,pcbSerializado,pcbSerializadoSize,0);

	t_pcb* pcb = malloc(sizeof(t_pcb));

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
void serializarStack(void** pcbSerializado,t_list* indiceStack){
	log_info(loggerConPantalla, "Serializando Stack");
	int i,j;
	t_nodoStack* node;

		for(i = 0; i < indiceStack->elements_count;i++){
			node = (t_nodoStack*) list_get(indiceStack, i);
			memcpy(pcbSerializado, &node->args->elements_count, sizeof(int));
			pcbSerializado += sizeof(int);

			for(j = 0; j < node->args->elements_count; j++){
				memcpy(pcbSerializado, list_get(node->args, j), sizeof(t_posMemoria));
				pcbSerializado += sizeof(t_posMemoria);
			}

			memcpy(pcbSerializado, &node->vars->elements_count, sizeof(int));
			pcbSerializado += sizeof(int);

			for(j = 0; j < node->vars->elements_count; j++){
				memcpy(pcbSerializado, list_get(node->vars,j), sizeof(t_variable));
				pcbSerializado += sizeof(t_variable);
			}

			memcpy(pcbSerializado, &node->retVar, sizeof(int));
			pcbSerializado += sizeof(int);

			memcpy(pcbSerializado, node->retVar, sizeof(t_posMemoria));
			pcbSerializado += sizeof(t_posMemoria);
		}
}
int calcularPcbSerializadoSize(t_pcb* pcb){
	log_info(loggerConPantalla, "Calculando tamano del PCB ---- PID: %d", pcb->pid);
	return sizeof(int)*5 + sizeof(t_puntero_instruccion) + calcularIndiceCodigoSize(pcb->cantidadInstrucciones) + calcularIndiceEtiquetasSize(pcb->cantidadEtiquetas) + calcularIndiceStackSize(pcb->indiceStack);
}
int calcularIndiceStackSize(t_list* indiceStack){
	log_info(loggerConPantalla, "Calculando tamano del Stack");
	int i;
	int stackSize=0;
	t_nodoStack* node;
	for(i=0;i<indiceStack->elements_count;i++){
			node = list_get(indiceStack,i);
		stackSize+= sizeof(int) + node->args->elements_count * sizeof(t_posMemoria) + sizeof(int)+  node->vars->elements_count * sizeof(t_variable) + sizeof(int) +sizeof(t_posMemoria);
	}
	return stackSize;
}

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		loggerSinPantalla = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		loggerConPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}
